#!/bin/bash

readonly ServerConfig=/etc/irods/server_config.json
readonly UserIrodsDir=/var/lib/irods/.irods
readonly UserIrodsFile="$UserIrodsDir"/irods_environment.json


main()
{
  configure_irods_server
  chmod 600 "$ServerConfig"
  mkdir --mode 0700 "$UserIrodsDir"
  configure_irods_user
  chmod 0600 "$UserIrodsFile"
}


# Update the iRODS server_config.json file.
configure_irods_server()
{
  set_server_config_field string icat_host "$IRODS_IES"
  set_server_config_field string zone_name "$IRODS_ZONE_NAME"
  set_server_config_field string default_resource_name "$IRODS_LOCAL_RESOURCE"
  set_server_config_field integer zone_port "$IRODS_PORT"
  set_server_config_field string zone_user "$IRODS_ADMIN_NAME"
  set_server_config_field string zone_auth_scheme native
}


# populate the irods environment for this server instance
configure_irods_user()
{
  cat <<EOF > "$UserIrodsFile"
{
  "irods_host": "$IRODS_HOST",
  "irods_port": $IRODS_PORT,
  "irods_default_resource": "$IRODS_LOCAL_RESOURCE",
  "irods_home": "/$IRODS_ZONE_NAME/home/$IRODS_ADMIN_NAME",
  "irods_cwd": "/$IRODS_ZONE_NAME/home/$IRODS_ADMIN_NAME",
  "irods_user_name": "$IRODS_ADMIN_NAME",
  "irods_zone_name": "$IRODS_ZONE_NAME",
  "irods_client_server_negotiation": "request_server_negotiation",
  "irods_client_server_policy": "CS_NEG_REFUSE",
  "irods_encryption_key_size": 32,
  "irods_encryption_salt_size": 8,
  "irods_encryption_num_hash_rounds": 16,
  "irods_encryption_algorithm": "AES-256-CBC",
  "irods_default_hash_scheme": "SHA256",
  "irods_match_hash_policy": "compatible",
  "irods_server_control_plane_port": $CONTROL_PLANE_PORT,
  "irods_server_control_plane_key": "$CONTROL_PLANE_KEY",
  "irods_server_control_plane_encryption_num_hash_rounds": 16,
  "irods_server_control_plane_encryption_algorithm": "AES-256-CBC",
  "irods_maximum_size_for_single_buffer_in_megabytes": 32,
  "irods_default_number_of_transfer_threads": 4,
  "irods_transfer_buffer_size_for_parallel_transfer_in_megabytes": 4
}
EOF
}


set_server_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  python /var/lib/irods/packaging/update_json.py "$ServerConfig" "$type" "$field" "$value"
}


set -e

main
