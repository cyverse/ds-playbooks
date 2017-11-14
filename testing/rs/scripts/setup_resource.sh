#!/bin/bash -e

readonly PackagingDir=/var/lib/irods/packaging
readonly ServerConfig=/etc/irods/server_config.json

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


set_server_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  "$Python" "$PackagingDir"/update_json.py "$ServerConfig" "$type" "$field" "$value"
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

printf '===================================================================\n'

printf 'Updating server_config.json...\n'
set_server_config_field string icat_host "$icatHost"
set_server_config_field string zone_name "$icatZone"
set_server_config_field string default_resource_name "$IRODS_LOCAL_RESOURCE"

printf 'Running irods_setup.pl...\n'
cd iRODS
perl /tmp/irods_setup_rs.pl "$IRODS_ZONE_PASSWORD"
