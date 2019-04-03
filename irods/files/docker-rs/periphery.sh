#!/bin/bash


main()
{
  local cmd="$*"

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

  local rescLoc
  rescLoc=$(hostname)

  local resc
  for resc in $(iquest '%s' "SELECT RESC_NAME WHERE RESC_LOC = '$rescLoc'")
  do
    if [[ "$resc" =~ ^CAT_NO_ROWS_FOUND ]]
    then
      break
    fi

    printf 'bringing %s %s\n' "$resc" "$status"
    iadmin modresc "$resc" status "$status"
  done
}


set_server_secrets()
{
  jq_in_place \
    ".irods_control_plane_key |= \"$CYVERSE_DS_CONTROL_PLANE_KEY\"" \
    /var/lib/irods/.irods/irods_environment.json

  jq_in_place \
    ".negotiation_key          |= \"$CYVERSE_DS_NEGOTIATION_KEY\" |
     .server_control_plane_key |= \"$CYVERSE_DS_CONTROL_PLANE_KEY\" |
     .zone_key                 |= \"$CYVERSE_DS_ZONE_KEY\"" \
    /etc/irods/server_config.json
}


jq_in_place()
{
  local filter="$1"
  local file="$2"

  jq "$filter" "$file" | sponge "$file"
}


main "$@"
