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


get_database_config_field()
{
  local field="$1"

  "$Python" -c "import json; print json.load(open('$DatabaseConfig'))['$field']"
}


set_database_config_field()
{
  local type="$1"
  local field="$2"
  local value="$3"

  "$Python" "$PackagingDir"/update_json.py "$DatabaseConfig" "$type" "$field" "$value"
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

set_database_config_field string catalog_database_type "$DBMS_TYPE"
set_database_config_field string db_username "$dbUser"
set_database_config_field string db_host "$dbmsHost"
set_database_config_field integer db_port "$dbmsPort"
set_database_config_field string db_name "$dbName"

printf '\n'

# =-=-=-=-=-=-=-
# run irods_setup.pl
printf '\n'
printf -- '-----------------------------\n'
printf 'Running irods_setup.pl...\n'
cd iRODS
perl /tmp/irods_setup_ies.pl "$DBMS_TYPE" "$dbmsHost" "$dbmsPort" "$dbUser" "$dbPassword"
cd ..

# =-=-=-=-=-=-=-
# done
exit 0
