#! /bin/bash -e

# iRODS packaging directory
readonly PackagingDir=/var/lib/irods/packaging

readonly ServerConfig=/etc/irods/server_config.json
readonly UserIrodsDir=/var/lib/irods/.irods
readonly UserIrodsFile="$UserIrodsDir"/irods_environment.json


main()
{
  setup_irods_service_account
  setup_irods_configuration

  mkdir --parents "$IRODS_DEFAULT_VAULT"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$IRODS_DEFAULT_VAULT"

  configure_irods_server
  chmod 0600 "$ServerConfig"
  chown "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$ServerConfig"

  mkdir --mode 0700 "$UserIrodsDir"
  configure_irods_user
  chmod 0600 "$UserIrodsFile"
  chown --recursive "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$UserIrodsDir"

  mk_service_script
  chmod a+rx /service.sh
}


# Update the iRODS server_config.json file.
configure_irods_server()
{
  set_server_config_field string icat_host "$IRODS_IES"
  set_server_config_field string zone_name "$IRODS_ZONE_NAME"
  set_server_config_field string default_resource_name "$IRODS_LOCAL_RESOURCE"
  set_server_config_field integer zone_port "$IRODS_ZONE_PORT"
  set_server_config_field string zone_user "$IRODS_ZONE_USER"
  set_server_config_field string zone_auth_scheme native
}


# populate the irods environment for this server instance
configure_irods_user()
{
  cat <<EOF > "$UserIrodsFile"
{
  "irods_host": "$IRODS_HOST",
  "irods_port": $IRODS_ZONE_PORT,
  "irods_default_resource": "$IRODS_LOCAL_RESOURCE",
  "irods_home": "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER",
  "irods_cwd": "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER",
  "irods_user_name": "$IRODS_ZONE_USER",
  "irods_zone_name": "$IRODS_ZONE_NAME",
  "irods_client_server_negotiation": "request_server_negotiation",
  "irods_client_server_policy": "CS_NEG_REFUSE",
  "irods_encryption_key_size": 32,
  "irods_encryption_salt_size": 8,
  "irods_encryption_num_hash_rounds": 16,
  "irods_encryption_algorithm": "AES-256-CBC",
  "irods_default_hash_scheme": "SHA256",
  "irods_match_hash_policy": "compatible",
  "irods_server_control_plane_port": $IRODS_CONTROL_PLANE_PORT,
  "irods_server_control_plane_key": "$IRODS_CONTROL_PLANE_KEY",
  "irods_server_control_plane_encryption_num_hash_rounds": 16,
  "irods_server_control_plane_encryption_algorithm": "AES-256-CBC",
  "irods_maximum_size_for_single_buffer_in_megabytes": 32,
  "irods_default_number_of_transfer_threads": 4,
  "irods_transfer_buffer_size_for_parallel_transfer_in_megabytes": 4
}
EOF
}


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


mk_service_script()
{
  cat <<EOF | sed --file - /tmp/service.sh.template > /service.sh
s/\$IRODS_IES/$(escape $IRODS_IES)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
s/\$IRODS_SYSTEM_USER/$(escape $IRODS_SYSTEM_USER)/g
s/\$IRODS_ZONE_PORT/$(escape $IRODS_ZONE_PORT)/g
EOF
}


set_server_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  python /var/lib/irods/packaging/update_json.py "$ServerConfig" "$type" "$field" "$value"
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


# define service account for this installation
setup_irods_service_account()
{
  "$PackagingDir"/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF
}


set -e

main
