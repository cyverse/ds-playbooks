#!/bin/bash
#
# This script configures the IES.
#
# It requires the following environment variables to be defined.
#
# DB_NAME                     The name of the DB iRODS will use.
# DB_PASSWORD                 The password used to authenticate DB_USER within
#                             PostgreSQL.
# DB_USER                     The DBMS user iRODS will use to connect to DB_NAME
#                             DB.
# DBMS_HOST                   The FQDN or IP address of the PostgreSQL server
# DBMS_PORT                   The TCP port the PostgreSQL will listen on.
# IRODS_CONTROL_PLANE_KEY     The encryption key required for communicating with
#                             the grid control plane.
# IRODS_CONTROL_PLANE_PORT    The port on which the control plane operates.
# IRODS_DEFAULT_RESOURCE      The name of the default resource to use
# IRODS_FIRST_EPHEMERAL_PORT  The beginning of the port range available for
#                             parallel transfer and reconnections.
# IRODS_HOST                  The FQDN or IP address of the server being
#                             configured.
# IRODS_LAST_EPHEMERAL_PORT   The end of the port range available for parallel
#                             transfer and reconnections.
# IRODS_NEGOTIATION_KEY       The shared encryption key used by the zone in
#                             advanced negotiation handshake a the beginning of
#                             a client connection
# IRODS_SCHEMA_VALIDATION     The URI for the schema used to validate the
#                             configuration files or 'off'.
# IRODS_SYSTEM_GROUP          The system group for the iRODS process
# IRODS_SYSTEM_USER           The system user for the iRODS process
# IRODS_ZONE_KEY              The shared secred used for authentication during
#                             server-to-server communication
# IRODS_ZONE_NAME             The name of the iRODS zone.
# IRODS_ZONE_PASSWORD         The password used to authenticate the
#                             IRODS_ZONE_USER user.
# IRODS_ZONE_PORT             The main TCP port used by the zone for
#                             communication.
# IRODS_ZONE_USER             The main rodsadmin user.


readonly CfgDir=/etc/irods
readonly DbCfg="$CfgDir"/database_config.json
readonly ServerCfg="$CfgDir"/server_config.json
readonly SvcAccount="$CfgDir"/service_account.config

readonly HomeDir=/var/lib/irods
readonly Odbc="$HomeDir"/.odbc.ini
readonly PgPass="$HomeDir"/.pgpass

readonly EnvDir="$HomeDir"/.irods
readonly EnvCfg="$EnvDir"/irods_environment.json


main()
{
  validate_32_byte_key "$IRODS_NEGOTIATION_KEY" "iRODS server's negotiation key"
  validate_32_byte_key "$IRODS_CONTROL_PLANE_KEY" 'Control Plane key'

  mk_svc_account
  ensure_ownership "$SvcAccount"

  populate_server_cfg
  ensure_ownership "$ServerCfg"

  populate_db_cfg
  ensure_ownership "$DbCfg"

  mkdir --parents --mode=0700 "$EnvDir"
  ensure_ownership "$EnvDir"

  mk_irods_env
  ensure_ownership "$EnvCfg"

  read -r -a drivers < <(/var/lib/irods/packaging/find_odbc_postgres.sh)
  prepare_odbc "${drivers[0]}" > "$Odbc"
  ensure_ownership "$Odbc"

  printf '*:%d:%s:%s:%s\n' "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD" > "$PgPass"
  ensure_ownership "$PgPass"

  prepare_svc_script > /service.sh
  chmod a+rx /service.sh
}


ensure_ownership()
{
  local fsEntity="$1"

  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$fsEntity"
  chmod u+rw "$fsEntity"
  chmod go= "$fsEntity"
}


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


get_cfg_field()
{
  local cfgFile="$1"
  local field="$2"

  python -c "import json; print json.load(open('$cfgFile'))['$field']"
}


mk_irods_env()
{
  local algorithm=$(get_cfg_field "$ServerCfg" server_control_plane_encryption_algorithm)
  local numRounds=$(get_cfg_field "$ServerCfg" server_control_plane_encryption_num_hash_rounds)

  printf '{}' > "$EnvCfg"
  set_cfg_field "$EnvCfg" string irods_client_server_negotiation request_server_negotiation
  set_cfg_field "$EnvCfg" string irods_client_server_policy CS_NEG_REFUSE
  set_cfg_field "$EnvCfg" string irods_cwd /"$IRODS_ZONE_NAME"/home/"$IRODS_ZONE_USER"
  set_cfg_field "$EnvCfg" string irods_default_hash_scheme SHA256
  set_cfg_field "$EnvCfg" integer irods_default_number_of_transfer_threads 4
  set_cfg_field "$EnvCfg" string irods_default_resource "$IRODS_DEFAULT_RESOURCE"
  set_cfg_field "$EnvCfg" string irods_encryption_algorithm AES-256-CBC
  set_cfg_field "$EnvCfg" integer irods_encryption_key_size 32
  set_cfg_field "$EnvCfg" integer irods_encryption_num_hash_rounds 16
  set_cfg_field "$EnvCfg" integer irods_encryption_salt_size 8
  set_cfg_field "$EnvCfg" string irods_home /"$IRODS_ZONE_NAME"/home/"$IRODS_ZONE_USER"
  set_cfg_field "$EnvCfg" string irods_host "$IRODS_HOST"
  set_cfg_field "$EnvCfg" string irods_match_hash_policy compatible
  set_cfg_field "$EnvCfg" integer irods_maximum_size_for_single_buffer_in_megabytes 32
  set_cfg_field "$EnvCfg" integer irods_port "$IRODS_ZONE_PORT"
  set_cfg_field "$EnvCfg" string irods_server_control_plane_encryption_algorithm "$algorithm"
  set_cfg_field "$EnvCfg" integer irods_server_control_plane_encryption_num_hash_rounds "$numRounds"
  set_cfg_field "$EnvCfg" string irods_server_control_plane_key "$IRODS_CONTROL_PLANE_KEY"
  set_cfg_field "$EnvCfg" integer irods_server_control_plane_port "$IRODS_CONTROL_PLANE_PORT"
  set_cfg_field "$EnvCfg" integer irods_transfer_buffer_size_for_parallel_transfer_in_megabytes 4
  set_cfg_field "$EnvCfg" string irods_user_name "$IRODS_ZONE_USER"
  set_cfg_field "$EnvCfg" string irods_zone_name "$IRODS_ZONE_NAME"
}


mk_svc_account()
{
  # define service account for this installation
  "$HomeDir"/packaging/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF
}


populate_db_cfg()
{
  set_cfg_field "$DbCfg" string catalog_database_type postgres
  set_cfg_field "$DbCfg" string db_host "$DBMS_HOST"
  set_cfg_field "$DbCfg" string db_name "$DB_NAME"
  set_cfg_field "$DbCfg" integer db_port "$DBMS_PORT"
  set_cfg_field "$DbCfg" string db_username "$DB_USER"
}


populate_server_cfg()
{
  set_cfg_field "$ServerCfg" string default_resource_name "$IRODS_DEFAULT_RESOURCE"
  set_cfg_field "$ServerCfg" string icat_host localhost
  set_cfg_field "$ServerCfg" string negotiation_key "$IRODS_NEGOTIATION_KEY"
  set_cfg_field "$ServerCfg" string schema_validation_base_uri "$IRODS_SCHEMA_VALIDATION"
  set_cfg_field "$ServerCfg" string server_control_plane_key "$IRODS_CONTROL_PLANE_KEY"
  set_cfg_field "$ServerCfg" integer server_control_plane_port "$IRODS_CONTROL_PLANE_PORT"
  set_cfg_field "$ServerCfg" integer server_port_range_end "$IRODS_LAST_EPHEMERAL_PORT"
  set_cfg_field "$ServerCfg" integer server_port_range_start "$IRODS_FIRST_EPHEMERAL_PORT"
  set_cfg_field "$ServerCfg" string zone_key "$IRODS_ZONE_KEY"
  set_cfg_field "$ServerCfg" string zone_name "$IRODS_ZONE_NAME"
  set_cfg_field "$ServerCfg" integer zone_port "$IRODS_ZONE_PORT"
  set_cfg_field "$ServerCfg" string zone_user "$IRODS_ZONE_USER"

  # Remove default resource directory
  sed --in-place '/"default_resource_directory"/d' "$ServerCfg"
}


prepare_odbc()
{
  local driver="$1"

  cat <<EOINI
[postgres]
Driver=$driver
Debug=0
CommLog=0
Servername=$DBMS_HOST
Database=$DB_NAME
ReadOnly=no
Ksqo=0
Port=$DBMS_PORT
EOINI
}


prepare_svc_script()
{
  cat <<EOF | sed --file - /tmp/service.sh.template
s/\$DBMS_HOST/$(escape $DBMS_HOST)/g
s/\$DBMS_PORT/$(escape $DBMS_PORT)/g
s/\$IRODS_SYSTEM_USER/$(escape $IRODS_SYSTEM_USER)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
EOF
}


set_cfg_field()
{
  local cfgFile="$1"
  local type="$2"
  local field="$3"
  local value="$4"

  python "$HomeDir"/packaging/update_json.py "$cfgFile" "$type" "$field" "$value"
}


validate_32_byte_key()
{
  local keyVal="$1"
  local keyName="$2"

  # check length (must equal 32)
  if [ ${#keyVal} -ne 32 ]
  then
    printf '%s needs to be 32 bytes long\n' "$keyName" >&2
    return 1
  fi
}


set -e

main
