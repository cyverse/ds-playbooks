export ENV_NAME=dstesting

export DBMS_HOST="$ENV_NAME"_dbms_1

export IRODS_IES="$ENV_NAME"_ies_1
export IRODS_IES_SYSTEM_GROUP=irods_ies
export IRODS_LAST_EPHEMERAL_PORT=20009
export IRODS_RS6_NAME=rs_centos6
export IRODS_RS7_NAME=rs_centos7
export IRODS_SCHEMA_VALIDATION=off
export IRODS_VAULT=/var/lib/irods/Vault
export IRODS_ZONE_NAME=iplant

export IRODS_DEFAULT_RESOURCE="$IRODS_RS6_NAME"
export IRODS_RS6_HOST="$ENV_NAME"_"$IRODS_RS6_NAME"_1
export IRODS_RS7_HOST="$ENV_NAME"_"$IRODS_RS7_NAME"_1
