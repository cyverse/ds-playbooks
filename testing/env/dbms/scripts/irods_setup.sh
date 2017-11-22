#!/bin/bash -e


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


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
