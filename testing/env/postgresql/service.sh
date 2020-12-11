#! /bin/bash
#
# Usage:
#  service (start|stop)
#
# This script starts or stops the postgresSQL server inside the container.

set -e


main()
{
  if [ "$#" -lt 1 ]
  then
    printf 'Requires either "start" or "stop" as its first parameter\n' >&2
    return 1
  fi

  local action="$1"

  if [ "$action" != start -a "$action" != stop ]
  then
    printf 'Unknown command "%s"\n' "$action" >&2
    return 1
  fi

  if id --user postgres; then
    gosu postgres pg_ctl -w "$action"
  fi
}


main "$@"
