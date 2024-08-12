#!/bin/bash
#
# This script creates the ICAT DB.
#
# It requires the following environment variables to be defined.
#
# DB_NAME              The name of the DB iRODS will use.
# DB_PASSWORD          The password used to authenticate DB_USER within
#                      PostgreSQL.
# DB_USER              The DBMS user iRODS will use to connect to DB_NAME DB.
# DBMS_PORT            The TCP port the PostgreSQL will listen on.
# IRODS_RESOURCES      A list of unixfilesystem storage resource definitions.
#                      See below.
# IRODS_ZONE_NAME      The name of the iRODS zone.
# IRODS_ZONE_PASSWORD  The password used to authenticate the IRODS_ZONE_USER
#                      user.
# IRODS_ZONE_USER      The main rodsadmin user.
#
# Resource Definition List Syntax:
#
# Each definition in the list is separated by a space (' '). A definition is a
# triple where each value is separated by a colon (':').  Here's its form:
# NAME:SERVER:VAULT. NAME is the name of the resource. SERVER is the hostname
# or IP address of the server managing the filesystem underlying the resource.
# VAULT is the absolute path to the vault on the filesystem.
#
# TODO The current syntax of IRODS_RESOURCES doesn't allow spaces or colons in
# vault path names. Please change its syntax.

set -e

if [[ "$OSTYPE" == "darwin"* ]]
then
  ExecName=$(greadlink -f "$0")
else
  ExecName=$(readlink --canonicalize "$0")
fi
readonly ExecName


main()
{
  local baseDir
  baseDir=$(dirname "$ExecName")

  local sqlData="$baseDir"/values.sql

  printf 'Preparing %s\n' "$sqlData"
  mk_cfg_sql \
      "$baseDir" "$IRODS_ZONE_NAME" "$IRODS_ZONE_USER" "$IRODS_ZONE_PASSWORD" "$IRODS_RESOURCES" \
    > "$sqlData"

  printf 'Starting PostgreSQL server\n'
  pg_ctlcluster 12 main start > /dev/null

  printf 'Initializing %s database ...\n' "$DB_NAME"
  init_db "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD" "$sqlData"

  printf 'Stopping PostgreSQL server\n'
  pg_ctlcluster 12 main stop > /dev/null
}


# escapes / and \ for sed script
escape_for_sed()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


expand_template()
{
  local zoneName="$1"
  local zoneUser="$2"
  local zonePasswd="$3"
  local template="$4"

  cat <<EOF | sed --file - "$template"
s/ZONE_NAME_TEMPLATE/$(escape_for_sed "$zoneName")/g
s/ADMIN_NAME_TEMPLATE/$(escape_for_sed "$zoneUser")/g
s/ADMIN_PASSWORD_TEMPLATE/$(escape_for_sed "$zonePasswd")/g
EOF
}


exec_sql()
{
  local port="$1"
  local db="$2"
  local user="$3"
  local passwd="$4"

  PGPASSWORD="$passwd" psql --port "$port" --user "$user" "$db" > /dev/null
}


init_db()
{
  local dbmsPort="$1"
  local name="$2"
  local user="$3"
  local passwd="$4"
  local sqlData="$5"

  local sqlDir
  sqlDir=$(dirname "$sqlData")

  printf '\tCreating %s database\n' "$name"
  psql --command "CREATE DATABASE \"$name"\" > /dev/null

  printf '\tCreating admin user %s\n' "$user"
  psql --command "CREATE USER $user WITH PASSWORD '$passwd'" > /dev/null
  psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$name\" TO $user" > /dev/null

  printf '\tCreating database tables\n'
  exec_sql "$dbmsPort" "$name" "$user" "$passwd" < "$sqlDir"/tables.sql

  printf '\tInitializing data\n'
  exec_sql "$dbmsPort" "$name" "$user" "$passwd" < "$sqlData"
}


mk_cfg_sql()
{
  local sqlDir="$1"
  local zone="$2"
  local admName="$3"
  local admPasswd="$4"
  local rescs="$5"

  local nowTs
  nowTs=$(date '+%s');

  cat "$sqlDir"/sys-values.sql
  expand_template "$zone" "$admName" "$admPasswd" "$sqlDir"/config-values.sql.template

  local id=9101

  while IFS=: read -r name server vault
  do
    mk_resc_insert "$zone" "$id" "$name" "$server" "$vault" "$nowTs"
    ((id++))
  done < <(tr ' ' '\n' <<< "$rescs")
}


mk_resc_insert()
{
  local zoneName="$1"
  local rescId="$2"
  local rescName="$3"
  local server="$4"
  local vault="$5"
  local createTs="$6"

  cat <<EOF
INSERT INTO R_RESC_MAIN (
  resc_id, resc_name,   zone_name,   resc_type_name,     resc_class_name, resc_net,  resc_def_path,
  free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts,    modify_ts,
  resc_context)
VALUES (
  $rescId, '$rescName', '$zoneName', 'unixfilesystem', 'cache',         '$server', '$vault',
  '2000000',  '0$createTs',  '',        '',        'up',        '0$createTs', '0$createTs',
  'minimum_free_space_for_create_in_bytes=1048576');
EOF
}


main "$@"
