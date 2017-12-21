#!/bin/bash -e

readonly CfgDir=/etc/irods
readonly DbCfg="$CfgDir"/database_config.json
readonly ServerCfg="$CfgDir"/server_config.json

readonly HomeDir=/var/lib/irods
readonly Odbc="$HomeDir"/.odbc.ini
readonly PgPass="$HomeDir"/.pgpass

readonly EnvDir="$HomeDir"/.irods
readonly EnvCfg="$EnvDir"/irods_environment.json


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
  local controlPlanePort=$(get_cfg_field "$ServerCfg" server_control_plane_port)
  local irodsUser=$(get_cfg_field "$ServerCfg" zone_user)
  local key=$(get_cfg_field "$ServerCfg" server_control_plane_key)
  local numRounds=$(get_cfg_field "$ServerCfg" server_control_plane_encryption_num_hash_rounds)
  local resc=$(get_cfg_field "$ServerCfg" default_resource_name)
  local zone=$(get_cfg_field "$ServerCfg" zone_name)
  local zonePort=$(get_cfg_field "$ServerCfg" zone_port)

  # Create .irods/irods_environment.json
  printf '{}' > "$EnvCfg"
  set_cfg_field "$EnvCfg" string irods_client_server_negotiation request_server_negotiation
  set_cfg_field "$EnvCfg" string irods_client_server_policy CS_NEG_REFUSE
  set_cfg_field "$EnvCfg" string irods_cwd /"$zone"/home/"$irodsUser"
  set_cfg_field "$EnvCfg" string irods_default_hash_scheme SHA256
  set_cfg_field "$EnvCfg" integer irods_default_number_of_transfer_threads 4
  set_cfg_field "$EnvCfg" string irods_default_resource "$resc"
  set_cfg_field "$EnvCfg" string irods_encryption_algorithm AES-256-CBC
  set_cfg_field "$EnvCfg" integer irods_encryption_key_size 32
  set_cfg_field "$EnvCfg" integer irods_encryption_num_hash_rounds 16
  set_cfg_field "$EnvCfg" integer irods_encryption_salt_size 8
  set_cfg_field "$EnvCfg" string irods_home /"$zone"/home/"$irodsUser"
  set_cfg_field "$EnvCfg" string irods_host "$IRODS_HOST"
  set_cfg_field "$EnvCfg" string irods_match_hash_policy compatible
  set_cfg_field "$EnvCfg" integer irods_maximum_size_for_single_buffer_in_megabytes 32
  set_cfg_field "$EnvCfg" integer irods_port "$zonePort"
  set_cfg_field "$EnvCfg" string irods_server_control_plane_encryption_algorithm "$algorithm"
  set_cfg_field "$EnvCfg" integer irods_server_control_plane_encryption_num_hash_rounds "$numRounds"
  set_cfg_field "$EnvCfg" string irods_server_control_plane_key "$key"
  set_cfg_field "$EnvCfg" integer irods_server_control_plane_port "$controlPlanePort"
  set_cfg_field "$EnvCfg" integer irods_transfer_buffer_size_for_parallel_transfer_in_megabytes 4
  set_cfg_field "$EnvCfg" string irods_user_name "$irodsUser"
  set_cfg_field "$EnvCfg" string irods_zone_name "$zone"
}


prepare_odbc()
{
  cat <<EOINI
[postgres]
Driver=/usr/pgsql-9.3/lib/psqlodbc.so
Debug=0
CommLog=0
Servername=$dbmsHost
Database=$dbName
ReadOnly=no
Ksqo=0
Port=$dbmsPort
EOINI
}


populate_db_cfg()
{
  local dbmsHost="$1"
  local dbmsPort="$2"
  local dbName="$3"
  local dbUser="$4"

  set_cfg_field "$DbCfg" string catalog_database_type "$DBMS_TYPE"
  set_cfg_field "$DbCfg" string db_host "$dbmsHost"
  set_cfg_field "$DbCfg" string db_name "$dbName"
  set_cfg_field "$DbCfg" integer db_port "$dbmsPort"
  set_cfg_field "$DbCfg" string db_username "$dbUser"
}


set_cfg_field()
{
  local cfgFile="$1"
  local type="$2"
  local field="$3"
  local value="$4"

  python "$HomeDir"/packaging/update_json.py "$cfgFile" "$type" "$field" "$value"
}


setup_irods_database()
{
  local dbName=
  local dbPassword=
  local dbUser=
  local dbmsHost=
  local dbmsPort=

  read -r dbmsHost
  read -r dbmsPort
  read -r dbName
  read -r dbUser
  read -r -s dbPassword

  set_cfg_field "$ServerCfg" string icat_host localhost
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$ServerCfg"
  chmod 0600 "$ServerCfg"

  populate_db_cfg "$dbmsHost" "$dbmsPort" "$dbName" "$dbUser"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$DbCfg"
  chmod 0600 "$DbCfg"

  mkdir --parents --mode=0700 "$EnvDir"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$EnvDir"

  mk_irods_env
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$EnvCfg"
  chmod 0600 "$EnvCfg"

  prepare_odbc "$dbmsHost" "$dbmsPort" "$dbName" > "$Odbc"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$Odbc"
  chmod 0600 "$Odbc"

  printf '*:%d:%s:%s:%s\n' "$dbmsPort" "$dbName" "$dbUser" "$dbPassword" > "$PgPass"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$PgPass"
  chmod 0600 "$PgPass"
}


update_server_config()
{
  validate_32_byte_key "$IRODS_NEGOTIATION_KEY" "iRODS server's negotiation key"
  validate_32_byte_key "$IRODS_CONTROL_PLANE_KEY" 'Control Plane key'

  set_cfg_field "$ServerCfg" string zone_name "$IRODS_ZONE_NAME"
  set_cfg_field "$ServerCfg" integer zone_port "$IRODS_ZONE_PORT"
  set_cfg_field "$ServerCfg" integer server_port_range_start "$IRODS_FIRST_EPHEMERAL_PORT"
  set_cfg_field "$ServerCfg" integer server_port_range_end "$IRODS_LAST_EPHEMERAL_PORT"
  set_cfg_field "$ServerCfg" string zone_user "$IRODS_ZONE_USER"
  set_cfg_field "$ServerCfg" string zone_key "$IRODS_ZONE_KEY"
  set_cfg_field "$ServerCfg" string negotiation_key "$IRODS_NEGOTIATION_KEY"
  set_cfg_field "$ServerCfg" integer server_control_plane_port "$IRODS_CONTROL_PLANE_PORT"
  set_cfg_field "$ServerCfg" string server_control_plane_key "$IRODS_CONTROL_PLANE_KEY"
  set_cfg_field "$ServerCfg" string schema_validation_base_uri "$IRODS_SCHEMA_VALIDATION"
  set_cfg_field "$ServerCfg" string icat_host $(hostname)
  set_cfg_field "$ServerCfg" string default_resource_name "$IRODS_DEFAULT_RESOURCE"

  # Remove default resource directory
  sed --in-place '/"default_resource_directory"/d' "$ServerCfg"
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

# define service account for this installation
"$HomeDir"/packaging/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF

# configure iRODS
update_server_config

# setup database
setup_irods_database <<EOF
$DBMS_HOST
$DBMS_PORT
$DB_NAME
$DB_USER
$DB_PASSWORD
EOF

# create bootstrap.sh
cat <<EOF | sed --file - /tmp/service.sh.template > /service.sh
s/\$DBMS_HOST/$(escape $DBMS_HOST)/g
s/\$DBMS_PORT/$(escape $DBMS_PORT)/g
s/\$IRODS_SYSTEM_USER/$(escape $IRODS_SYSTEM_USER)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
EOF

chmod a+rx /service.sh
