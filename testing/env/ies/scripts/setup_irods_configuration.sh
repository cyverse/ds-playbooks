#!/bin/bash -e

# set some paths
readonly PackagingDir=/var/lib/irods/packaging
readonly ServerConfig=/etc/irods/server_config.json
readonly SetupIRODSCfg=/tmp/"$USER"/setup_irods_configuration.flag

# detect correct python version
if type -P python2 1> /dev/null
then
  readonly Python=$(type -P python2)
else
  readonly Python=$(type -P python)
fi


get_server_config_field()
{
  local field="$1"

  "$Python" -c "import json; print json.load(open('$ServerConfig'))['$field']"
}


set_server_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  "$Python" "$PackagingDir"/update_json.py "$ServerConfig" "$type" "$field" "$value"
}


request_cfg_value()
{
  local cfgVar="$1"
  local question="$2"
  local default="$3"

  printf '%s [%s]: ' "$question" "$default"
  read -r "$cfgVar"
  printf '\n'

  if [ -z ${!cfgVar} ]
  then
    printf -v "$cfgVar" '%s' "$default"
  fi
}


# get temp file from prior run, if it exists
mkdir --parents /tmp/"$USER"

if [ -f "$SetupIRODSCfg" ]
then
  # have run this before, read the existing config files
  zone=$(get_server_config_field zone_name)
  port=$(get_server_config_field zone_port)
  rangeStart=$(get_server_config_field server_port_range_start)
  rangeEnd=$(get_server_config_field server_port_range_end)
  localZoneKey=$(get_server_config_field zone_key)
  negotiationKey=$(get_server_config_field negotiation_key)
  controlPlanePort=$(get_server_config_field server_control_plane_port)
  controlPlaneKey=$(get_server_config_field server_control_plane_key)
  validationBaseURI=$(get_server_config_field schema_validation_base_uri)
  adminName=$(get_server_config_field zone_user)
  status=loop
else
  # no temp file, this is the first run
  status=firstpass
fi

# strip cruft from zone_key
tmp="${localZoneKey#\"}"
tmp="${tmp%\,}"
localZoneKey="${tmp%\"}"

if [ -z "$localZoneKey" ]
then
  unset localZoneKey
fi

# strip cruft from negotiation_key
tmp="${negotiationKey#\"}"
tmp="${tmp%\,}"
negotiationKey="${tmp%\"}"

if [ -z "$negotiationKey" ]
then
  unset negotiationKey
fi

readonly PreviousID="$localZoneKey"
readonly PreviousKey="$negotiationKey"

# ask human for irods environment
printf '===================================================================\n'
printf '\n'
printf 'You are installing iRODS.\n'
printf '\n'
printf 'The iRODS server cannot be started until it has been configured.\n'
printf '\n'


while [ "$status" != complete ]
do
  # ask for configuration values
  request_cfg_value zone "iRODS server's zone" "${zone-tempZone}"
  request_cfg_value port "iRODS server's port" "${port-1247}"
  request_cfg_value rangeStart 'iRODS port range (begin)' "${rangeStart-20000}"
  request_cfg_value rangeEnd 'iRODS port range (end)' "${rangeEnd-20199}"
  request_cfg_value localZoneKey "iRODS server's zone_key" "${localZoneKey-TEMPORARY_zone_key}"

  # get negotiation_key
  while true
  do
    request_cfg_value val \
                      "iRODS server's negotiation_key" \
                      "${negotiationKey-TEMPORARY_32byte_negotiation_key}"

    # check length (must equal 32)
    if [ ${#val} -ne 32 ]
    then
      printf '   *** negotiation_key must be exactly 32 bytes ***\n'
      printf '\n'
      printf '   %s <- %d bytes\n' "$negotiationKey" "$negotiationKeyLen"
      printf '   ________________________________ <- 32 bytes\n'
      printf '\n'
    else
      negotiationKey="$val"
      break
    fi
  done

  request_cfg_value controlPlanePort 'Control Plane port' "${controlPlanePort-1248}"

  # get control plane key
  while true
  do
    request_cfg_value val 'Control Plane key' "${controlPlaneKey-TEMPORARY__32byte_ctrl_plane_key}"

    # check length (must equal 32)
    if [ ${#val} -ne 32 ]
    then
      printf '   *** control plane key must be exactly 32 bytes ***\n'
      printf '\n'
      printf '   %s <- %d bytes\n' "$controlPlaneKey" "$controlPlaneKeyLen"
      printf '   ________________________________ <- 32 bytes\n'
      printf '\n'
    else
      controlPlaneKey="$val"
      break
    fi
  done

  request_cfg_value validationBaseURI \
                    "Schema Validation Base URI (or 'off')" \
                    "${validationBaseURI-https://schemas.irods.org/configuration}"

  request_cfg_value adminName "iRODS server's administrator username" "${adminName-rods}"

  printf '\n'
  printf '\n'

  # confirm
  printf -- '-------------------------------------------\n'
  printf 'iRODS Zone:                 %s\n' "$zone"
  printf 'iRODS Port:                 %s\n' "$port"
  printf 'Range (Begin):              %s\n' "$rangeStart"
  printf 'Range (End):                %s\n' "$rangeEnd"
  printf 'zone_key:                   %s\n' "$localZoneKey"
  printf 'negotiation_key:            %s\n' "$negotiationKey"
  printf 'Control Plane Port:         %s\n' "$controlPlanePort"
  printf 'Control Plane Key:          %s\n' "$controlPlaneKey"
  printf 'Schema Validation Base URI: %s\n' "$validationBaseURI"
  printf 'Administrator Username:     %s\n' "$adminName"
  printf 'Administrator Password:     Not Shown\n'
  printf -- '-------------------------------------------\n'
  request_cfg_value confirm 'Please confirm these settings' yes

  if [ -z "$confirm" -o "$confirm" == y -o "$confirm" == Y -o "$confirm" == yes ]
  then
    status=complete
  else
    status=loop
  fi

  printf '\n'
done

touch "$SetupIRODSCfg"

# update existing server_config.json
printf 'Updating %s...\n' "$ServerConfig"

# zone name
set_server_config_field string zone_name "$zone"

# everything else
set_server_config_field integer zone_port "$port"
set_server_config_field integer server_port_range_start "$rangeStart"
set_server_config_field integer server_port_range_end "$rangeEnd"
set_server_config_field string zone_user "$adminName"
set_server_config_field string zone_key "$localZoneKey"
set_server_config_field string negotiation_key "$negotiationKey"
set_server_config_field integer server_control_plane_port "$controlPlanePort"
set_server_config_field string server_control_plane_key "$controlPlaneKey"
set_server_config_field string schema_validation_base_uri "$validationBaseURI"
set_server_config_field string icat_host $(hostname)
set_server_config_field string default_resource_name "$IRODS_DEFAULT_RESOURCE"

# Remove default resource directory
sed --in-place '/"default_resource_directory"/d' "$ServerConfig"
