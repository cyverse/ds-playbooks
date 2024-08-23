#!/usr/bin/env bash
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
# IRODS_DELAY_SERVER   The name of the iRODS delay server
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

set -o errexit -o nounset -o pipefail

ExecName=$(readlink --canonicalize "$0")
readonly ExecName


main()
{
  local baseDir
  baseDir=$(dirname "$ExecName")

  local sql="$baseDir"/icat.sql

  printf 'Preparing %s\n' "$sql"
  mk_cfg_sql \
      "$baseDir" \
      "$IRODS_ZONE_NAME" \
      "$IRODS_ZONE_USER" \
      "$IRODS_ZONE_PASSWORD" \
      "$IRODS_DELAY_SERVER" \
      "$IRODS_RESOURCES" \
    > "$sql"

  printf 'Starting PostgreSQL server\n'
  pg_ctlcluster 12 main start > /dev/null

  printf 'Initializing %s database ...\n' "$DB_NAME"
  init_db "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD" "$sql"

  printf 'Stopping PostgreSQL server\n'
  pg_ctlcluster 12 main stop > /dev/null
}


# escapes / and \ for sed script
escape_for_sed()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  var="${var/$'\n'/\\n}"
  printf '%s' "${var//\//\\/}"
}


expand_template()
{
  local zoneName="$1"
  local zoneUser="$2"
  local zonePasswd="$3"
  local delayServer="$4"
  local rescs="$5"
  local template="$6"

  cat <<EOF | sed --file - "$template"
s/ZONE_NAME_TEMPLATE/$(escape_for_sed "$zoneName")/g
s/ADMIN_NAME_TEMPLATE/$(escape_for_sed "$zoneUser")/g
s/ADMIN_PASSWORD_TEMPLATE/$(escape_for_sed "$zonePasswd")/g
s/DELAY_LEADER_TEMPLATE/$(escape_for_sed "$delayServer")/g
s/DELAY_SUCCESSOR_TEMPLATE//g
s/CUSTOM_RESC_TEMPLATE/$(escape_for_sed "$rescs")/g
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
  local sql="$5"

  local sqlDir
  sqlDir=$(dirname "$sql")

  printf '\tCreating %s database\n' "$name"
  psql --command "CREATE DATABASE \"$name"\" > /dev/null

  printf '\tCreating admin user %s\n' "$user"
  psql --command "CREATE USER $user WITH PASSWORD '$passwd'" > /dev/null
  psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$name\" TO $user" > /dev/null

  printf '\tCreating database\n'
  exec_sql "$dbmsPort" "$name" "$user" "$passwd" < "$sql"
}


mk_cfg_sql()
{
  local sqlDir="$1"
  local zone="$2"
  local admName="$3"
  local admPasswd="$4"
  local delayServer="$5"
  local rescs="$6"

  local rescsSql
  rescsSql="$(mk_rescs_sql "$zone" "$(date +%s)" "$rescs")"

  expand_template \
    "$zone" "$admName" "$admPasswd" "$delayServer" "$rescsSql" "$sqlDir"/icat.sql.template
}


mk_rescs_sql()
{
  local zoneName="$1"
  local createTs="$2"
  local rescs="$3"

  local rescId=9100
  local rescName
  local zoneName
  local rescTypeName=unixfilesystem
  local rescClassName=cache
  local rescNet
  local rescDefPath
  local freeSpace=2000000
  local freeSpaceTs="$createTs"
  local rescInfo=''
  local rComment=''
  local rescStatus=up
  local createTs
  local modifyTs="$createTs"
  local rescChildren='\N'
  local rescContext='minimum_free_space_for_create_in_bytes=1048576'
  local rescParent='\N'
  local rescObjCount=0
  local rescParentContext='\N'
  local modifyTsMillis='000'

  while IFS=: read -r rescName rescNet rescDefPath
  do
    ((rescId++))

    printf '%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$rescId" \
      "$rescName" \
      "$zoneName" \
      "$rescTypeName" \
      "$rescClassName" \
      "$rescNet" \
      "$rescDefPath" \
      "$freeSpace" \
      "$freeSpaceTs" \
      "$rescInfo" \
      "$rComment" \
      "$rescStatus" \
      "$createTs" \
      "$modifyTs" \
      "$rescChildren" \
      "$rescContext" \
      "$rescParent" \
      "$rescObjCount" \
      "$rescParentContext" \
      "$modifyTsMillis"
  done < <(tr ' ' '\n' <<< "$rescs")
}


main "$@"
