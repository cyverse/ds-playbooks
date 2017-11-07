#!/bin/bash -e

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
$IRODS_DEFAULT_VAULT
$IRODS_ZONE_KEY
$IRODS_NEGOTIATION_KEY
$IRODS_CONTROL_PLANE_PORT
$IRODS_CONTROL_PLANE_KEY
$IRODS_SCHEMA_VALIDATION
$IRODS_ZONE_USER
yes
EOF

# if default vault path does not exist, create it with proper permissions
if [ ! -e "$IRODS_DEFAULT_VAULT" ]
then
  mkdir --parents "$IRODS_DEFAULT_VAULT"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$IRODS_DEFAULT_VAULT"
fi

# setup database
sudo -i -u "$IRODS_SYSTEM_USER" \
    DB_NAME="$DB_NAME" DBMS_TYPE="$DBMS_TYPE" /tmp/setup_irods_database.sh \
<<EOF
$DBMS_HOST
$DBMS_PORT
$DB_NAME
$DB_USER
$DB_PASSWORD
yes
EOF

# init .irodsA
g++ -o /tmp/mk_irods_a --std c++0x /tmp/mk_irods_a.cpp -lcrypto
/tmp/mk_irods_a "$IRODS_ZONE_PASSWORD"

# create bootstrap.sh
sed "s/\\\$DBMS_HOST/$DBMS_HOST/g" /tmp/bootstrap.template > /bootstrap.sh
chmod a+rx /bootstrap.sh
