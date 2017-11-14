#!/bin/bash -e

# escapes / and \ for sed script
escape()
{
  local var="$1"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


# define service account for this installation
/var/lib/irods/packaging/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF

# configure iRODS
sudo -i -u "$IRODS_SYSTEM_USER" /tmp/setup_irods_configuration.sh <<EOF
$IRODS_ZONE_NAME
$IRODS_ZONE_PORT
$IRODS_FIRST_EPHEMERAL_PORT
$IRODS_LAST_EPHEMERAL_PORT
$IRODS_ZONE_KEY
$IRODS_NEGOTIATION_KEY
$IRODS_CONTROL_PLANE_PORT
$IRODS_CONTROL_PLANE_KEY
$IRODS_SCHEMA_VALIDATION
$IRODS_ZONE_USER
yes
EOF

# setup database
sudo -i -u "$IRODS_SYSTEM_USER" \
    DB_NAME="$DB_NAME" DBMS_TYPE="$DBMS_TYPE" IRODS_HOST="$IRODS_HOST" \
    /tmp/setup_irods_database.sh \
<<EOF
$DBMS_HOST
$DBMS_PORT
$DB_NAME
$DB_USER
$DB_PASSWORD
yes
EOF

# create bootstrap.sh
cat <<EOF | sed --file - /tmp/bootstrap.template > /bootstrap.sh
s/\$DBMS_HOST/$(escape $DBMS_HOST)/g
s/\$DBMS_PORT/$(escape $DBMS_PORT)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
EOF

chmod a+rx /bootstrap.sh
