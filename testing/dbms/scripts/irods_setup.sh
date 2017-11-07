#!/bin/bash -e

perl /tmp/convertSql.pl "$DBMS_TYPE" /tmp

g++ -o /tmp/scramble --std c++0x /tmp/scramble.cpp -lcrypto
readonly ScrambledPassword=$(/tmp/scramble "$IRODS_ZONE_PASSWORD")

# prepare SQL from template
readonly ICATSetupValues=/tmp/icatSetupValues.sql

printf 'Preparing %s...\n' "$ICATSetupValues"

cat <<EOF | sed --file - "$ICATSetupValues".template > "$ICATSetupValues"
s/ZONE_NAME_TEMPLATE/$IRODS_ZONE_NAME/g
s/ADMIN_NAME_TEMPLATE/$IRODS_ZONE_USER/g
s/HOSTNAME_TEMPLATE/$IRODS_IES/g
s;RESOURCE_DIR_TEMPLATE;$IRODS_DEFAULT_VAULT;g
s/ADMIN_PASSWORD_TEMPLATE/$ScrambledPassword/g
s/RESOURCE_NAME_TEMPLATE/$IRODS_DEFAULT_RESOURCE/g
EOF

/usr/pgsql-9.3/bin/pg_ctl -w start
psql --command "CREATE DATABASE \"$DB_NAME"\"
psql --command "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD'"
psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$DB_NAME\" TO $DB_USER"
perl /tmp/irods_setup_dbms.pl "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD"
/usr/pgsql-9.3/bin/pg_ctl -w stop
