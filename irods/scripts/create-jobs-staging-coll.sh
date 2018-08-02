#! /bin/bash
#
# This script ensures that the staging collection exists.
#
# Usage:
#  create-jobs-staging-coll.sh
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
  if [ "$#" -lt 1 ]
  then
    printf 'requires one input parameter\n' >&2
    return 1
  fi
  
  local zone="$1"

  trap finish_up EXIT

  local resp
  if ! resp=$(iquest "select COLL_ID where COLL_NAME = '/$zone/jobs'")
  then
    return 1
  fi

  if [[ "$resp" =~ CAT_NO_ROWS_FOUND ]]
  then
    if ! imkdir /"$zone"/jobs
    then
      return 1
    fi

    ChangedSomething=true
  fi
}


set -u
main "$@"
