#!/bin/bash

readonly PackagingDir=/var/lib/irods/packaging
readonly ServerConfig=/etc/irods/server_config.json
readonly UserIrodsDir=/var/lib/irods/.irods


# Update the iRODS server_config.json file.
configure_irods_server()
{
  local icatHost="$1"
  local zoneName="$2"

  set_server_config_field string icat_host "$icatHost"
  set_server_config_field string zone_name "$icatZone"
  set_server_config_field string default_resource_name "$IRODS_LOCAL_RESOURCE"
  set_server_config_field integer zone_port "$IRODS_PORT"
  set_server_config_field string zone_user "$IRODS_ADMIN_NAME"
  set_server_config_field string zone_auth_scheme native

  chmod 600 "$ServerConfig"
  mkdir --mode 0700 "$UserIrodsDir"
}


# populate the irods environment for this server instance
configure_irods_user()
{
  local icatZone="$1"

  local userIrodsFile="$UserIrodsDir"/irods_environment.json

  cat <<EOF > "$userIrodsFile"
{
  "irods_host": "$IRODS_HOST",
  "irods_port": $IRODS_PORT,
  "irods_default_resource": "$IRODS_LOCAL_RESOURCE",
  "irods_home": "/$icatZone/home/$IRODS_ADMIN_NAME",
  "irods_cwd": "/$icatZone/home/$IRODS_ADMIN_NAME",
  "irods_user_name": "$IRODS_ADMIN_NAME",
  "irods_zone_name": "$icatZone",
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

  chmod 0600 "$userIrodsFile"
}


request_cfg_value()
{
  local var="$1"
  local question="$2"

  printf '%s [%s]: ' "$question" "${!var}"
  read -r ans
  printf '\n'

  if [ -n "$ans" ]
  then
    printf -v "$var" '%s' "$ans"
  fi
}


set_server_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  "$Python" "$PackagingDir"/update_json.py "$ServerConfig" "$type" "$field" "$value"
}


# detect correct python version
if type -P python2 1> /dev/null
then
  readonly Python=$(type -P python2)
else
  readonly Python=$(type -P python)
fi

# Set defaults
icatHost=ies
icatZone=tempZone


cd "$PackagingDir"/..

# no temp file, this is the first run
status=firstpass

printf '===================================================================\n'
printf '\n'
printf 'You are installing an iRODS resource server.  Resource servers\n'
printf 'cannot be started until they have been properly configured to\n'
printf 'communicate with a live iCAT server.\n'
printf '\n'

while [ "$status" != complete ]
do
  # ask for configuration values
  request_cfg_value icatHost "iCAT server's hostname"
  request_cfg_value icatZone "iCAT server's ZoneName"

  # confirm
  printf -- '-------------------------------------------\n'
  printf 'iCAT Host:    %s\n' "$icatHost"
  printf 'iCAT Zone:    %s\n' "$icatZone"
  printf -- '-------------------------------------------\n'

  confirm=yes
  request_cfg_value confirm 'Please confirm these settings'

  if [ "$confirm" == y -o "$confirm" == Y -o "$confirm" == yes ]
  then
    status=complete
  else
    status=loop
  fi

  printf '\n'
  printf '\n'
done

configure_irods_server "$icatHost" "$icatZone"
configure_irods_user "$icatZone"
