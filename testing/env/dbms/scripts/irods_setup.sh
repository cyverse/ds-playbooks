#!/bin/bash -e


convert_sql()
{
  local dbmsType="$1"
  local tmplDir="$2"

  if [ "$dbmsType" != oracle -a "$dbmsType" != postgres -a "$dbmsType" != mysql ]
  then
    printf 'Usage postgres|oracle|mysql\n'
    printf 'Invalid argument\n' >&2
    return 1
  fi

  local cwd="$(pwd)"

  cd "$tmplDir"

  local cmd="cpp -D'$dbmsType' icatSysTables.sql.pp | grep -v '^#' > icatSysTables.sql"

  printf 'running: %s\n' "$cmd"
  eval "$cmd" > /dev/null
  local cmdStat="$?"

  if [ "$cmdStat" -ne 0 ]
  then
    printf 'The following command failed:\n'
    printf '%s\n' "$cmd"
    printf 'Exit code = %d\n' "$cmdStat"
    printf 'command failed\n' >&2
    cd "$cwd"
    return 1
  fi

  printf 'Preprocess icatSysTables.sql.pp to icatSysTables.sql\n'
  cd "$cwd"
}


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


convert_sql "$DBMS_TYPE" /tmp

# prepare SQL from template
readonly ICATSetupValues=/tmp/icatSetupValues.sql

printf 'Preparing %s...\n' "$ICATSetupValues"

cat <<EOF | sed --file - "$ICATSetupValues".template > "$ICATSetupValues"
s/ZONE_NAME_TEMPLATE/$(escape $IRODS_ZONE_NAME)/g
s/ADMIN_NAME_TEMPLATE/$(escape $IRODS_ZONE_USER)/g
s/ADMIN_PASSWORD_TEMPLATE/$(escape $IRODS_ZONE_PASSWORD)/g
EOF

id=9101

while IFS=: read -r name server vault
do
  cat <<EOF
INSERT INTO R_RESC_MAIN (
  resc_id, resc_name, zone_name,          resc_type_name,   resc_class_name, resc_net,
  resc_def_path, free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts,
  modify_ts)
VALUES (
  $id,     '$name',   '$IRODS_ZONE_NAME', 'unixfilesystem', 'cache',         '$server',
  '$vault',      '',         '',            '',        '',        '',          '1170000000',
  '1170000000');
EOF

((id++))
done < <(tr ' ' '\n' <<< "$IRODS_RESOURCES") >> "$ICATSetupValues"

/usr/pgsql-9.3/bin/pg_ctl -w start
psql --command "CREATE DATABASE \"$DB_NAME"\"
psql --command "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD'"
psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$DB_NAME\" TO $DB_USER"
perl /tmp/irods_setup_dbms.pl "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD"
/usr/pgsql-9.3/bin/pg_ctl -w stop
