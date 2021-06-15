#!/bin/bash

export ENV_NAME=dstesting
export DOMAIN="$ENV_NAME"_default

export DBMS_HOST="$ENV_NAME"_dbms_configured_1."$DOMAIN"

export IRODS_IES_SYSTEM_GROUP=irods_ies
export IRODS_LAST_EPHEMERAL_PORT=20009
export IRODS_RS_CONF_NAME=rs_ingest
export IRODS_RS_UNCONF_NAME=rs_repl
export IRODS_SCHEMA_VALIDATION=off
export IRODS_VAULT=/var/lib/irods/Vault
export IRODS_ZONE_NAME=testing

export IRODS_DEFAULT_RESOURCE="$IRODS_RS_CONF_NAME"
export IRODS_IES_CONF_HOST="$ENV_NAME"_ies_configured_1."$DOMAIN"
export IRODS_IES_UNCONF_HOST="$ENV_NAME"_ies_unconfigured_1."$DOMAIN"
export IRODS_RS_CONF_HOST="$ENV_NAME"_rs_configured_1."$DOMAIN"
export IRODS_RS_UNCONF_HOST="$ENV_NAME"_rs_unconfigured_1."$DOMAIN"
