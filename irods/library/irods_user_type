#!/usr/bin/env bash
#
# An ansible module for managing iRODS user types. At the moment, it can only
# create them.
#
# Module Name:
#  irods_user_type
#
# Required Variables:
#  type  the type of user to create
#
# Optional Variables:
#  description  the description of the user type

# WANT_JSON


main()
{
  local varFile="$1"

  local type
  local description

  type=$(read_field "$varFile" type)
  description=$(read_field "$varFile" description '')

  local id
  id=$(quest "select TOKEN_ID where TOKEN_NAMESPACE = 'user_type' and TOKEN_NAME = '$type'")

  if [ -z "$id" ]
  then
    local errMsg
    if ! errMsg=$(iadmin at user_type "$type" '' "$description" 2>&1)
    then
      fail "$errMsg"
    fi

    succeed true
  fi

  local currentDesc
  currentDesc=$(quest "select TOKEN_VALUE2 where TOKEN_ID = '$id'")

  if [ "$description" != "$currentDesc" ]
  then
    fail "user type $type already in use for another purpose"
  fi

  succeed false
}


quest()
{
  local query="$*"

  local response
  if ! response=$(iquest --no-page '%s' "$query" 2>&1)
  then
    fail "$response"
  fi

  if ! [[ "$response" =~ ^CAT_NO_ROWS_FOUND ]]
  then
    printf '%s' "$response"
  fi
}


read_field()
{
  local varFile="$1"
  local field="$2"

  local defaultValue
  if [ "$#" -ge 3 ]
  then
    defaultValue="$3"
  fi

  local value
  if ! value=$(jq --raw-output ".$field // empty" < "$varFile" 2>&1)
  then
    fail "$value"
  elif [ -z "$value" ]
  then
    if [ "${defaultValue:-}" ]
    then
      value="$defaultValue"
    else
      fail "variable '$field' must be defined"
    fi
  fi

  printf '%s' "$value"
}


fail()
{
  local msg="$*"

  # shellcheck disable=SC2016
  jq --compact-output --monochrome-output --null-input --arg msg "$msg" \
     '{failed: true, msg: $msg}' \
    >&2

  exit 1
}


succeed()
{
  local changed="$1"

  # shellcheck disable=SC2016
  jq --compact-output --monochrome-output --null-input --arg changed "$changed" \
     '{changed: ($changed == "true")}' \
    >&2

  exit 0
}


set -e
main "$@" 2>&1
