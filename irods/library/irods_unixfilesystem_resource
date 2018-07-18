#!/bin/bash

Changed=false


notify_changed()
{
  printf '%s' "$Changed"
}
trap notify_changed EXIT


main()
{
  local resc="$1"
  local server="$2"
  local vault="$3"
  local context="$4"

  local rescType
  rescType=$(iquest '%s' "select RESC_TYPE_NAME where RESC_NAME = '$resc'")

  if [ "$rescType" = "CAT_NO_ROWS_FOUND: Nothing was found matching your query" ]
  then
    define_resource "$resc" "$server" "$vault" "$context"
  else
    update_resource "$resc" "$server" "$vault" "$context"
  fi
}


define_resource()
{
  local resc="$1"
  local server="$2"
  local vault="$3"
  local context="$4"

  iadmin mkresc "$resc" unixfilesystem "$server":"$vault" "$context" > /dev/null
  Changed=true
  iadmin modresc "$resc" status down
  set_free_space "$resc" "$vault"
}


set_free_space()
{
  local resc="$1"
  local vault="$2"

  local freeSpace
  freeSpace=$(df --portability --block-size 1 "$vault" | tail -1 | awk '{ print $4 }')
  iadmin modresc "$resc" freespace "$freeSpace"
  Changed=true
}


update_resource()
{
  local resc="$1"
  local server="$2"
  local vault="$3"
  local context="$4"

  local rescLoc
  rescLoc=$(iquest '%s:%s' "select RESC_LOC, RESC_VAULT_PATH where RESC_NAME = '$resc'")

  if [ "$rescType" != unixfilesystem -o "$rescLoc" != "$server":"$vault" ]
  then
    printf '%s exists with incorrect definition "%s %s"\n' "$resc" "$rescType" "$rescLoc" >&2
    return 1
  fi

  local currentContext
  currentContext=$(iadmin lr "$resc" | sed --quiet 's/^resc_context: //p')

  if [ "$context" != "$currentContext" ]
  then
    iadmin modresc "$resc" context "$context"
    Changed=true
  fi

  local freeSpace
  freeSpace=$(iquest '%s' "select RESC_FREE_SPACE where RESC_NAME = '$resc'")

  if [ -z "$freeSpace" ]
  then
    set_free_space "$resc" "$vault"
  fi
}


set -eu -o pipefail
main "$@"
