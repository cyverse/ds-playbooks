#!/bin/bash
#
# This script creates the ds-service user for DE jobs. This user will have an
# empty password.
#
# This script is intended to be executed on an IES by the service account.
#
# Usage:
#  create-de-jobs-user.sh USERNAME
#
# Parameters:
#  USERNAME  The name of the user account to create
#
# Returns:
#  It writes 'true' to standard output if at least one permission was changed,
#  otherwise it writes 'false'.

ChangedSomething=false


finish_up()
{
  local exitCode="$?"

  printf %s "$ChangedSomething"
  exit "$exitCode"
}


main()
{
  trap finish_up EXIT

  if [ "$#" -lt 1 ]
  then
    printf 'requires the name of the user as a parameter\n' >&2
    return 1
  fi

  local user="$1"

  if [ -z "$user" ]
  then
    printf 'user name cannot be empty\n' >&2
  fi

  if ! ensure_account "$user"
  then
    return 1
  fi

  if ! ensure_empty_password "$user"
  then
    return 1
  fi
}


ensure_account()
{
  local user="$1"

  local oldUser
  if ! oldUser=$(get_existing_user)
  then
    return 1
  fi

  if [ "$user" = "$oldUser" ]
  then
    return 0
  fi

  local userInfo
  if ! userInfo=$(iadmin lu "$user")
  then
    return 1
  fi

  if [ "$userInfo" != 'No rows found' ]
  then
    printf '%s is already in use\n' "$user" >&2
    return 1
  fi

  if ! iadmin mkuser "$user" ds-service
  then
    return 1
  fi

  ChangedSomething=true

  if ! iadmin moduser "$user" info 'DE job'
  then
    return 1
  fi

  if [ -n "$oldUser" ]
  then
    if ! iadmin rmuser "$oldUser"
    then
      return 1
    fi
  fi
}


ensure_empty_password()
{
  local user="$1"

  if ! IRODS_USER_NAME="$user" IRODS_AUTHENTICATION_FILE=/dev/null iinit -e <<< '' > /dev/null
  then
    if ! iadmin moduser "$user" password ''
    then
      return 1
    fi

    ChangedSomething=true
  fi
}


get_existing_user()
{
  local oldUser
  if ! oldUser=$(iquest '%s' "select USER_NAME where USER_INFO = 'DE job'")
  then
    return 1
  fi

  if [[ "$oldUser" =~ CAT_NO_ROWS_FOUND: ]]
  then
    oldUser=
  fi

  printf '%s' "$oldUser"
}


set -u
main "$@"
