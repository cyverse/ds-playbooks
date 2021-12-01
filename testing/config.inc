#!/bin/bash

export ENV_NAME=dstesting
export DOMAIN="$ENV_NAME"_default

export DBMS_HOST="$ENV_NAME"_dbms_configured_1."$DOMAIN"

export IRODS_LAST_EPHEMERAL_PORT=20009
export IRODS_PROVIDER_SYSTEM_GROUP=irods_provider
export IRODS_RES_CONF_CENTOS_NAME=terraref
export IRODS_RES_CONF_UBUNTU_NAME=ingestRes
export IRODS_RES_UNCONF_NAME=replRes
export IRODS_SCHEMA_VALIDATION=off
export IRODS_VAULT=/var/lib/irods/Vault
export IRODS_ZONE_NAME=testing

export IRODS_CONSUMER_CONF_CENTOS_HOST="$ENV_NAME"_consumer_configured_centos_1."$DOMAIN"
export IRODS_CONSUMER_CONF_UBUNTU_HOST="$ENV_NAME"_consumer_configured_ubuntu_1."$DOMAIN"
export IRODS_CONSUMER_UNCONF_HOST="$ENV_NAME"_consumer_unconfigured_1."$DOMAIN"
export IRODS_DEFAULT_RESOURCE="$IRODS_RES_CONF_UBUNTU_NAME"
export IRODS_PROVIDER_CONF_HOST="$ENV_NAME"_provider_configured_1."$DOMAIN"
export IRODS_PROVIDER_UNCONF_HOST="$ENV_NAME"_provider_unconfigured_1."$DOMAIN"
