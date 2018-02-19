#! /bin/bash
#
# This script configures a resource server.
#
# It requires the following environment variables to be defined.
#
# IRODS_CONTROL_PLANE_KEY     The encryption key required for communicating with
#                             the grid control plane.
# IRODS_CONTROL_PLANE_PORT    The port on which the control plane operates.
# IRODS_DEFAULT_RESOURCE      The name of the default resource to use
# IRODS_DEFAULT_VAULT         The absolute path to the vault of the resource
#                             server.
# IRODS_FIRST_EPHEMERAL_PORT  The beginning of the port range available for
#                             parallel transfer and reconnections.
# IRODS_HOST                  The FQDN or IP address of the server being
#                             configured.
# IRODS_IES                   The FQDN or IP address of the IES.
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
readonly ServerCfg="$CfgDir"/server_config.json
readonly SvcAccount="$CfgDir"/service_account.config

readonly HomeDir=/var/lib/irods

readonly EnvDir="$HomeDir"/.irods
readonly EnvCfg="$EnvDir"/irods_environment.json

readonly PackagingDir="$HomeDir"/packaging


main()
{
  validate_32_byte_key "$IRODS_NEGOTIATION_KEY" "iRODS server's negotiation key"
  validate_32_byte_key "$IRODS_CONTROL_PLANE_KEY" 'Control Plane key'

  mk_svc_account
  ensure_ownership "$SvcAccount"

  setup_irods_configuration

  mkdir --parents "$IRODS_DEFAULT_VAULT"
  ensure_ownership "$IRODS_DEFAULT_VAULT"

  populate_server_cfg
  ensure_ownership "$ServerCfg"

  mkdir --parents --mode=0700 "$EnvDir"
  ensure_ownership "$EnvDir"

  mk_irods_env
  ensure_ownership "$EnvCfg"

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
  "$PackagingDir"/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF
}


populate_server_cfg()
{
  set_cfg_field "$ServerCfg" string icat_host "$IRODS_IES"
  set_cfg_field "$ServerCfg" string zone_name "$IRODS_ZONE_NAME"
  set_cfg_field "$ServerCfg" string default_resource_name "$IRODS_DEFAULT_RESOURCE"
  set_cfg_field "$ServerCfg" integer zone_port "$IRODS_ZONE_PORT"
  set_cfg_field "$ServerCfg" string zone_user "$IRODS_ZONE_USER"
  set_cfg_field "$ServerCfg" string zone_auth_scheme native
}


prepare_svc_script()
{
  cat <<EOF | sed --file - /tmp/service.sh.template
s/\$IRODS_IES/$(escape $IRODS_IES)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
s/\$IRODS_SYSTEM_USER/$(escape $IRODS_SYSTEM_USER)/g
s/\$IRODS_ZONE_PORT/$(escape $IRODS_ZONE_PORT)/g
EOF
}


set_cfg_field()
{
  local cfgFile="$1"
  local type="$2"
  local field="$3"
  local value="$4"

  python "$PackagingDir"/update_json.py "$cfgFile" "$type" "$field" "$value"
}


# configure irods
setup_irods_configuration()
{
  sudo -i -u "$IRODS_SYSTEM_USER" "$PackagingDir"/setup_irods_configuration.sh <<EOF
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
