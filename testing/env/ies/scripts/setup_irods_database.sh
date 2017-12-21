#!/bin/bash -e

readonly TopLevelDir=/var/lib/irods
readonly PackagingDir="$TopLevelDir"/packaging
readonly SetupIrodsDatabase=/tmp/"$USER"/setup_irods_database.flag
readonly DatabaseConfig=/etc/irods/database_config.json
readonly ServerConfig=/etc/irods/server_config.json

# detect correct python version
if type -P python2 1> /dev/null
then
  readonly Python=$(type -P python2)
else
  readonly Python=$(type -P python)
fi

# Set defaults
dbmsHost=localhost
dbmsPort=5432
dbUser=irods
dbPassword=temppassword
dbName=ICAT


cfg_db_user()
{
  local dbmsPort="$1"
  local dbName="$2"
  local dbAdminName="$3"
  local dbAdminPassword="$4"

  printf '*:%d:%s:%s:%s\n' "$dbmsPort" "$dbName" "$dbAdminName" "$dbAdminPassword" \
    > "$TopLevelDir"/.pgpass
}


cfg_irods_server()
{
  local dbmsType="$1"
  local dbAdminName="$2"
  local dbAdminPassword="$3"

  set_cfg_field "$ServerConfig" string icat_host localhost
  set_cfg_field "$ServerConfig" string zone_auth_scheme native
  chmod 0600 "$ServerConfig"

  set_cfg_field "$DatabaseConfig" string catalog_database_type "$dbmsType"
  set_cfg_field "$DatabaseConfig" string db_username "$dbAdminName"
  set_cfg_field "$DatabaseConfig" string db_password "$dbAdminPassword"
  chmod 0600 "$DatabaseConfig"
}


cfg_irods_user()
{
  local host=$(hostname --fqdn)
  local zonePort=$(get_cfg_field "$ServerConfig" zone_port)
  local resc=$(get_cfg_field "$ServerConfig" default_resource_name)
  local zone=$(get_cfg_field "$ServerConfig" zone_name)
  local admin=$(get_cfg_field "$ServerConfig" zone_user)
  local controlPlanePort=$(get_cfg_field "$ServerConfig" server_control_plane_port)
  local key=$(get_cfg_field "$ServerConfig" server_control_plane_key)
  local numRounds=$(get_cfg_field "$ServerConfig" server_control_plane_encryption_num_hash_rounds)
  local algorithm=$(get_cfg_field "$ServerConfig" server_control_plane_encryption_algorithm)

  local envDir="$TopLevelDir"/.irods
  mkdir "$envDir"
  chmod 0700 "$envDir"

  local envCfg="$envDir"/irods_environment.json
  printf '{}' > "$envCfg"
  set_cfg_field "$envCfg" string irods_host "$host"
  set_cfg_field "$envCfg" integer irods_port "$zonePort"
  set_cfg_field "$envCfg" string irods_default_resource "$resc"
  set_cfg_field "$envCfg" string irods_home /"$zone"/home/"$admin"
  set_cfg_field "$envCfg" string irods_cwd /"$zone"/home/"$admin"
  set_cfg_field "$envCfg" string irods_user_name "$admin"
  set_cfg_field "$envCfg" string irods_zone_name "$zone"
  set_cfg_field "$envCfg" string irods_client_server_negotiation request_server_negotiation
  set_cfg_field "$envCfg" string irods_client_server_policy CS_NEG_REFUSE
  set_cfg_field "$envCfg" integer irods_encryption_key_size 32
  set_cfg_field "$envCfg" integer irods_encryption_salt_size 8
  set_cfg_field "$envCfg" integer irods_encryption_num_hash_rounds 16
  set_cfg_field "$envCfg" string irods_encryption_algorithm AES-256-CBC
  set_cfg_field "$envCfg" string irods_default_hash_scheme SHA256
  set_cfg_field "$envCfg" string irods_match_hash_policy compatible
  set_cfg_field "$envCfg" integer irods_server_control_plane_port "$controlPlanePort"
  set_cfg_field "$envCfg" string irods_server_control_plane_key "$key"
  set_cfg_field "$envCfg" integer irods_server_control_plane_encryption_num_hash_rounds "$numRounds"
  set_cfg_field "$envCfg" string irods_server_control_plane_encryption_algorithm "$algorithm"
  set_cfg_field "$envCfg" integer irods_maximum_size_for_single_buffer_in_megabytes 32
  set_cfg_field "$envCfg" integer irods_default_number_of_transfer_threads 4
  set_cfg_field "$envCfg" integer irods_transfer_buffer_size_for_parallel_transfer_in_megabytes 4
  chmod 0600 "$envCfg"
}


create_odbc_ini()
{
  local dbmsHost="$1"
  local dbmsPort="$2"
  local dbName="$3"

  cat <<EOINI > "$TopLevelDir"/.odbc.ini
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


get_cfg_field()
{
  local cfgFile="$1"
  local field="$2"

  "$Python" -c "import json; print json.load(open('$cfgFile'))['$field']"
}


irods_setup_ies()
{
  local dbmsType="$1"
  local dbmsHost="$2"
  local dbmsPort="$3"
  local dbName="$4"
  local dbUser="$5"
  local dbPassword="$6"

  cfg_db_user "$dbmsPort" "$dbName" "$dbUser" "$dbPassword"
  create_odbc_ini "$dbmsHost" "$dbmsPort" "$dbName"
  cfg_irods_server "$dbmsType" "$dbUser" "$dbPassword"
  cfg_irods_user
}


set_cfg_field()
{
  local cfgFile="$1"
  local type="$2"
  local field="$3"
  local value="$4"

  "$Python" "$PackagingDir"/update_json.py "$cfgFile" "$type" "$field" "$value"
}


request_cfg_value()
{
  local dbVar="$1"
  local question="$2"

  printf '%s [%s]: ' "$question" "${!dbVar}"
  read -r ans
  printf '\n'

  if [ -n "$ans" ]
  then
    printf -v "$dbVar" '%s' "$ans"
  fi
}


# get into the top level directory
cd "$TopLevelDir"

# get temp file from prior run, if it exists
mkdir --parents /tmp/"$USER"

if [ -f "$SetupIrodsDatabase" ]
then
  # have run this before, read the existing config file
  dbmsHost=$(get_database_config_field db_host)
  dbmsPort=$(get_database_config_field db_port)
  dbName=$(get_database_config_field db_name)
  dbUser=$(get_database_config_field db_username)
  status=loop
else
  # no temp file, this is the first run
  status=firstpass
fi

printf '===================================================================\n'
printf '\n'
printf 'You are installing an iRODS database plugin.\n'
printf '\n'
printf 'The iRODS server cannot be started until its database\n'
printf 'has been properly configured.\n'
printf '\n'

while [ "$status" != complete ]
do
  # set default values from an earlier loop
  if [ "$status" != firstpass ]
  then
    LastDBUser="$dbUser"
  fi

  request_cfg_value dbmsHost "Database server's hostname or IP address"
  request_cfg_value dbmsPort "Database server's port"
  request_cfg_value dbName 'Database name'
  request_cfg_value dbUser 'Database user'

  # get db password, without showing on screen
  printf 'Database password: '
  read -r -s dbPassword
  printf '\n'
  printf '\n'

  # confirm
  printf -- '-------------------------------------------\n'
  printf 'Database Type:     %s\n' "$DBMS_TYPE"
  printf 'Hostname or IP:    %s\n' "$dbmsHost"
  printf 'Database Port:     %s\n' "$dbmsPort"
  printf 'Database Name:     %s\n' "$dbName"
  printf 'Database User:     %s\n' "$dbUser"
  printf 'Database Password: Not Shown\n'
  printf -- '-------------------------------------------\n'

  confirm=yes
  request_cfg_value confirm 'Please confirm these settings'

  if [ "$confirm" == "" -o "$confirm" == "y" -o "$confirm" == "Y" -o "$confirm" == "yes" ]
  then
    status="complete"
  else
    status="loop"
  fi

  printf '\n'
done

touch "$SetupIrodsDatabase"
printf '===================================================================\n'

# update database_config.json
printf 'Updating %s...\n' "$DatabaseConfig"

set_cfg_field "$DatabaseConfig" string catalog_database_type "$DBMS_TYPE"
set_cfg_field "$DatabaseConfig" string db_username "$dbUser"
set_cfg_field "$DatabaseConfig" string db_host "$dbmsHost"
set_cfg_field "$DatabaseConfig" integer db_port "$dbmsPort"
set_cfg_field "$DatabaseConfig" string db_name "$dbName"

printf '\n'

# =-=-=-=-=-=-=-
# run irods_setup.pl
printf '\n'
printf -- '-----------------------------\n'
printf 'Running irods_setup.pl...\n'
cd iRODS
irods_setup_ies "$DBMS_TYPE" "$dbmsHost" "$dbmsPort" "$dbName" "$dbUser" "$dbPassword"
cd ..

# =-=-=-=-=-=-=-
# done
exit 0
