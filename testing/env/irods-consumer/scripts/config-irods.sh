#! /bin/bash
#
# This script configures a catalog consumer hosting a storage resource.
#
# It requires the following environment variables to be defined.
#
# IRODS_CATALOG_PROVIDER      the FQDN or IP address of the catalog service 
#                             provider
# IRODS_CONTROL_PLANE_KEY     the encryption key required for communicating with
#                             the grid control plane
# IRODS_CONTROL_PLANE_PORT    the port on which the control plane operates
# IRODS_DEFAULT_RESOURCE      the name of the default resource to use
# IRODS_DEFAULT_VAULT         the absolute path to the vault of the resource
#                             server
# IRODS_FIRST_EPHEMERAL_PORT  the beginning of the port range available for
#                             parallel transfer and reconnections
# IRODS_HOST                  the FQDN or IP address of the server being
#                             configured
# IRODS_LAST_EPHEMERAL_PORT   the end of the port range available for parallel
#                             transfer and reconnections
# IRODS_NEGOTIATION_KEY       the shared encryption key used by the zone in
#                             advanced negotiation handshake a the beginning of
#                             a client connection
# IRODS_SCHEMA_VALIDATION     the URI for the schema used to validate the
#                             configuration files or 'off'
# IRODS_SYSTEM_GROUP          the system group for the iRODS process
# IRODS_SYSTEM_USER           the system user for the iRODS process
# IRODS_ZONE_KEY              the shared secred used for authentication during
#                             server-to-server communication
# IRODS_ZONE_NAME             the name of the iRODS zone.
# IRODS_ZONE_PASSWORD         the password used to authenticate the
#                             IRODS_ZONE_USER user
# IRODS_ZONE_PORT             the main TCP port used by the zone for
#                             communication
# IRODS_ZONE_USER             the main rodsadmin user

set -o errexit -o nounset -o pipefail

readonly CfgDir=/etc/irods
readonly HostAccessControlCfg="$CfgDir"/host_access_control_config.json
readonly HostsCfg="$CfgDir"/hosts_config.json
readonly ServerCfg="$CfgDir"/server_config.json
readonly SvcAccount="$CfgDir"/service_account.config

readonly HomeDir=/var/lib/irods
readonly Version="$HomeDir"/VERSION.json

readonly EnvDir="$HomeDir"/.irods
readonly EnvCfg="$EnvDir"/irods_environment.json


main()
{
  validate_32_byte_key "$IRODS_NEGOTIATION_KEY" "iRODS server's negotiation key"
  validate_32_byte_key "$IRODS_CONTROL_PLANE_KEY" 'Control Plane key'

  mkdir --parents "$IRODS_DEFAULT_VAULT"

  mk_svc_account
  mk_svc_account_cfg > "$SvcAccount"

  mk_server_cfg > "$ServerCfg"
  mk_host_access_control_cfg > "$HostAccessControlCfg"
  mk_hosts_cfg > "$HostsCfg"

  mk_version "$(date +%FT%T.000000)" > "$Version"
  
  mkdir "$EnvDir"
  mk_irods_env > "$EnvCfg"

  ensure_ownership "$HomeDir"
  ensure_ownership "$CfgDir"
  ensure_ownership "$IRODS_DEFAULT_VAULT"

  usermod --append --groups "$IRODS_SYSTEM_GROUP" root
}


ensure_ownership()
{
  local fsEntity="$1"

  chown --recursive "$IRODS_SYSTEM_USER":"$IRODS_SYSTEM_GROUP" "$fsEntity"
  chmod --recursive u+rw,go= "$fsEntity"
}


# define service account for this installation
mk_svc_account()
{
  groupadd --force --system "$IRODS_SYSTEM_GROUP"

  local gid homeParam
  if [[ -e /etc/centos-release ]]
  then
    gid="$IRODS_SYSTEM_GROUP"
    homeParam=--home-dir
  else
    if ! gid="$(sed --quiet 's/^'"$IRODS_SYSTEM_GROUP"':[^:]*:\([0-9]\+\).*/\1/p' /etc/group)"
    then
      return 1
    fi

    homeParam=--home
  fi

  local err
  if ! \
    err="$(
      adduser --system --gid="$gid" "$homeParam"=/var/lib/irods --shell=/bin/bash \
          "$IRODS_SYSTEM_USER" \
        2>&1 )"
  then
    local rc=$?
    if (( rc != 9 ))
    then
      echo "$err" >&2
      return $rc
    fi
  fi
}


mk_host_access_control_cfg()
{
  cat /var/lib/irods/packaging/host_access_control_config.json.template
}


mk_hosts_cfg()
{
  cat /var/lib/irods/packaging/hosts_config.json.template
}


mk_irods_env()
{
  jq --sort-keys --from-file /dev/stdin <(echo '{}') <<JQ
.irods_host = "$IRODS_HOST" |
.irods_port = $IRODS_ZONE_PORT |
.irods_user_name = "$IRODS_ZONE_USER" |
.irods_zone_name = "$IRODS_ZONE_NAME" |
.irods_client_server_negotiation = "request_server_negotiation" | 
.irods_client_server_policy = "CS_NEG_REFUSE" |
.irods_control_plane_key = "$IRODS_CONTROL_PLANE_KEY" |
.irods_control_plane_port = $IRODS_CONTROL_PLANE_PORT |
.irods_cwd = "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER" |
.irods_default_hash_scheme = "SHA256" |
.irods_default_resource = "$IRODS_DEFAULT_RESOURCE" |
.irods_encryption_algorithm = "AES-256-CBC" |
.irods_encryption_key_size = 32 |
.irods_encryption_num_hash_rounds = 16 |
.irods_encryption_salt_size = 8 |
.irods_home = "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER" |
.irods_match_hash_policy = "compatible" |
.schema_name = "service_account_environment" |
.schema_version = "v3"
JQ
}


mk_server_cfg()
{
  jq --sort-keys --from-file /dev/stdin /var/lib/irods/packaging/server_config.json.template <<JQ
.catalog_provider_hosts |= [ "$IRODS_CATALOG_PROVIDER" ] |
.catalog_service_role |= "consumer" |
.default_resource_name |= "$IRODS_DEFAULT_RESOURCE" |
.default_resource_directory |= "$IRODS_DEFAULT_VAULT" |
.negotiation_key |= "$IRODS_NEGOTIATION_KEY" |
.schema_validation_base_uri |= "$IRODS_SCHEMA_VALIDATION" |
.server_control_plane_key |= "$IRODS_CONTROL_PLANE_KEY" |
.server_control_plane_port |= $IRODS_CONTROL_PLANE_PORT |
.server_port_range_end |= $IRODS_LAST_EPHEMERAL_PORT |
.server_port_range_start |= $IRODS_FIRST_EPHEMERAL_PORT |
.zone_key |= "$IRODS_ZONE_KEY" |
.zone_name |= "$IRODS_ZONE_NAME" |
.zone_port |= $IRODS_ZONE_PORT |
.zone_user |= "$IRODS_ZONE_USER"
JQ
}


mk_svc_account_cfg()
{
  cat <<EOF
IRODS_SERVICE_ACCOUNT_NAME=$IRODS_SYSTEM_USER
IRODS_SERVICE_GROUP_NAME=$IRODS_SYSTEM_GROUP
EOF
}



mk_version()
{
  local installTime="$1"

  jq --sort-keys --from-file /dev/stdin /var/lib/irods/VERSION.json.dist <<JQ
.installation_time |= "$installTime" 
JQ
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


main "$@"
