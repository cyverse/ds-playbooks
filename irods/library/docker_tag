#!/bin/bash

# WANT_JSON

set -o nounset


main()
{
  local varFile="$1"

  local source
  source=$(jq --raw-output '.source // ""' < "$varFile")
  require source

  local target
  target=$(jq --raw-output '.target // ""' < "$varFile")
  require target

  local sourceId
  if ! sourceId=$(docker inspect --format '{{ .Id }}' "$source" 2> /dev/null)
  then
    fail "source image "$source" not found"
  fi

  local targetId
  if targetId=$(docker inspect --format '{{ .Id }}' "$target" 2> /dev/null)
  then
    if [[ "$sourceId" = "$targetId" ]]
    then
      succeed false
    fi
  fi

  local msg
  if ! msg=$(docker tag "$source" "$target" 2>&1)
  then
    fail "$msg"
  fi

  succeed true
}


require()
{
  local var="$1"

  if [[ -z "${!var}" ]]
  then
    fail "variable '$var' must be defined"
  fi
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


main "$@" 2>&1
