#!/bin/bash

Changed=false


main()
{
  local hierDef="$*"

  create_hierarchy "$hierDef"

  if [ "$Changed" = true ]
  then
    printf changed
  fi
}


add_child()
{
  local parent="$1"
  local child="$2"

  local curParent
  curParent=$(iquest '%s' "select RESC_PARENT where RESC_NAME = '$child'")

  if [ -z "$curParent" ]
  then
    iadmin addchildtoresc "$parent" "$child"
    Changed=true
  elif [ "$curParent" = "CAT_NO_ROWS_FOUND: Nothing was found matching your query" ]
  then
    printf 'child %s doesn""t exist\n' "$child" >&2
    return 1
  elif [ "$curParent" != "$parent" ]
  then
    printf 'The current parent of %s is %s, not %s\n' "$child" "$curParent" "$parent" >&2
    return 1
  fi
}


create_and_add_children()
{
  local parentName="$1"

  local childDef
  while IFS= read -r childDef
  do
    if [ -n "$childDef" ]
    then
      create_hierarchy "$childDef"

      local childName
      childName=$(jq --raw-output .name <<< "$childDef")

      add_child "$parentName" "$childName"
    fi
  done
}


create_coord_resc()
{
  local name="$1"
  local type="$2"
  local context="$3"

  local curType
  curType=$(iquest '%s' "select RESC_TYPE_NAME where RESC_NAME = '$name'")

  if [ "$curType" = "CAT_NO_ROWS_FOUND: Nothing was found matching your query" ]
  then
    iadmin mkresc "$name" "$type" '' "$context" > /dev/null
    iadmin modresc "$name" status down
    Changed=true
  elif [ "$curType" != "$type" ]
  then
    printf '%s exists with incorrect type %s\n' "$name" "$type" >&2
    return 1
  fi
}


create_hierarchy()
{
  local def="$*"

  local type
  type=$(jq --raw-output '.type // empty' <<< "$def")

  if [ -n "$type" ]
  then
    local name
    name=$(jq --raw-output .name <<< "$def")

    local context
    context=$(jq --raw-output '.context // empty' <<< "$def")

    create_coord_resc "$name" "$type" "$context"

    jq --compact-output --raw-output 'if .children then .children[] else empty end' <<< "$def" \
      | create_and_add_children "$name"
  fi
}


set -eu -o pipefail
main "$*"
