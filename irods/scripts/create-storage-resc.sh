#!/bin/bash


main()
{
  local resc="$1"
  local server="$2"
  local vault="$3"

  local rescType
  rescType=$(iquest '%s' "select RESC_TYPE_NAME where RESC_NAME = '$resc'")

  if [ "$rescType" = "CAT_NO_ROWS_FOUND: Nothing was found matching your query" ]
  then
    iadmin mkresc "$resc" unixfilesystem "$server":"$vault"
    printf changed
    return 0
  fi

  local rescLoc
  rescLoc=$(iquest '%s:%s' "select RESC_LOC, RESC_VAULT_PATH where RESC_NAME = '$resc'")

  if [ "$rescType" != unixfilesystem -o "$rescLoc" != "$server":"$vault" ]
  then
    printf '%s exists with incorrect definition "%s %s"\n' "$resc" "$rescType" "$rescLoc" >&2
    return 1
  fi
}


set -eu -o pipefail
main "$@"
