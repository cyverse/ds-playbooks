#!/bin/bash -e


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


set_server_config_field()
{
  local cfg="$1"
  local type="$2"
  local field="$3"
  local value="$4"

  python /var/lib/irods/packaging/update_json.py "$cfg" "$type" "$field" "$value"
}


update_server_config()
{
  local cfg=/etc/irods/server_config.json

  validate_32_byte_key "$IRODS_NEGOTIATION_KEY" "iRODS server's negotiation key"
  validate_32_byte_key "$IRODS_CONTROL_PLANE_KEY" 'Control Plane key'

  set_server_config_field "$cfg" string zone_name "$IRODS_ZONE_NAME"
  set_server_config_field "$cfg" integer zone_port "$IRODS_ZONE_PORT"
  set_server_config_field "$cfg" integer server_port_range_start "$IRODS_FIRST_EPHEMERAL_PORT"
  set_server_config_field "$cfg" integer server_port_range_end "$IRODS_LAST_EPHEMERAL_PORT"
  set_server_config_field "$cfg" string zone_user "$IRODS_ZONE_USER"
  set_server_config_field "$cfg" string zone_key "$IRODS_ZONE_KEY"
  set_server_config_field "$cfg" string negotiation_key "$IRODS_NEGOTIATION_KEY"
  set_server_config_field "$cfg" integer server_control_plane_port "$IRODS_CONTROL_PLANE_PORT"
  set_server_config_field "$cfg" string server_control_plane_key "$IRODS_CONTROL_PLANE_KEY"
  set_server_config_field "$cfg" string schema_validation_base_uri "$IRODS_SCHEMA_VALIDATION"
  set_server_config_field "$cfg" string icat_host $(hostname)
  set_server_config_field "$cfg" string default_resource_name "$IRODS_DEFAULT_RESOURCE"

  # Remove default resource directory
  sed --in-place '/"default_resource_directory"/d' "$cfg"
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
/var/lib/irods/packaging/setup_irods_service_account.sh <<EOF
$IRODS_SYSTEM_USER
$IRODS_SYSTEM_GROUP
EOF

# configure iRODS
update_server_config

# setup database
sudo -i -u "$IRODS_SYSTEM_USER" \
    DB_NAME="$DB_NAME" DBMS_TYPE="$DBMS_TYPE" IRODS_HOST="$IRODS_HOST" \
    /tmp/setup_irods_database.sh \
<<EOF
$DBMS_HOST
$DBMS_PORT
$DB_NAME
$DB_USER
$DB_PASSWORD
yes
EOF

# create bootstrap.sh
cat <<EOF | sed --file - /tmp/service.sh.template > /service.sh
s/\$DBMS_HOST/$(escape $DBMS_HOST)/g
s/\$DBMS_PORT/$(escape $DBMS_PORT)/g
s/\$IRODS_SYSTEM_USER/$(escape $IRODS_SYSTEM_USER)/g
s/\$IRODS_ZONE_PASSWORD/$(escape $IRODS_ZONE_PASSWORD)/g
EOF

chmod a+rx /service.sh
