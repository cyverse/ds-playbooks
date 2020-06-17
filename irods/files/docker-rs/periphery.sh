#!/bin/bash
#
# Usage:
#  periphery
#
# This program is meant to be called by the entry point script of the base
# container. See https://github.com/cyverse/docker-irods-rs#design for the
# details. When the `before_start` argument is passed in, it updates the iRODS
# configuration files with the required secret keys. When `after_start` is
# provided, it sets the status of its storage resource to `up`. Finally, when
# `before_stop` is passed in, it sets the status to `down`.  It ignores all
# other arguments.
#
# Ths program expects the following environment variables to be defined.
#
# IRODS_CONTROL_PLANE_KEY  the iRODS server control plane key
# IRODS_NEGOTIATION_KEY    the iRODS negotiation key
# IRODS_STORAGE_RES        the storage resource being managed
# IRODS_ZONE_KEY           the iRODS zone key


main()
{
  local cmd="$1"

  case "$cmd" in
    before_start)
      set_server_secrets
      ;;
    after_start)
      set_resource_status up
      ;;
    before_stop)
      set_resource_status down
      ;;
    after_stop)
      ;;
    *)
      ;;
  esac
}


set_resource_status()
{
  local status="$1"

  printf 'bringing %s %s\n' "$IRODS_STORAGE_RES" "$status"
  iadmin modresc "$IRODS_STORAGE_RES" status "$status"
}


set_server_secrets()
{
  jq_in_place \
    ".irods_server_control_plane_key |= \"$IRODS_CONTROL_PLANE_KEY\"" \
    /var/lib/irods/.irods/irods_environment.json

  jq_in_place \
    ".negotiation_key          |= \"$IRODS_NEGOTIATION_KEY\" |
     .server_control_plane_key |= \"$IRODS_CONTROL_PLANE_KEY\" |
     .zone_key                 |= \"$IRODS_ZONE_KEY\"" \
    /etc/irods/server_config.json
}


jq_in_place()
{
  local filter="$1"
  local file="$2"

  jq "$filter" "$file" | sponge "$file"
}


main "$@"
