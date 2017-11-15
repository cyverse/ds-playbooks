#!/bin/bash -e

# escapes / and \ for sed script
escape()
{
  local var="$1"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


perl /tmp/convertSql.pl "$DBMS_TYPE" /tmp

# prepare SQL from template
readonly ICATSetupValues=/tmp/icatSetupValues.sql

printf 'Preparing %s...\n' "$ICATSetupValues"

IFS=: read -r resourceName resourceServer vault <<<"$IRODS_RESOURCES"

cat <<EOF | sed --file - "$ICATSetupValues".template > "$ICATSetupValues"
s/ZONE_NAME_TEMPLATE/$(escape $IRODS_ZONE_NAME)/g
s/ADMIN_NAME_TEMPLATE/$(escape $IRODS_ZONE_USER)/g
s/HOSTNAME_TEMPLATE/$(escape $resourceServer)/g
s/RESOURCE_DIR_TEMPLATE/$(escape $vault)/g
s/ADMIN_PASSWORD_TEMPLATE/$(escape $IRODS_ZONE_PASSWORD)/g
s/RESOURCE_NAME_TEMPLATE/$(escape $resourceName)/g
EOF

/usr/pgsql-9.3/bin/pg_ctl -w start
psql --command "CREATE DATABASE \"$DB_NAME"\"
psql --command "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD'"
psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$DB_NAME\" TO $DB_USER"
perl /tmp/irods_setup_dbms.pl "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD"
/usr/pgsql-9.3/bin/pg_ctl -w stop
