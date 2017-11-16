#! /bin/bash

main()
{
  if [ "$#" -lt 1 ]
  then
    printf 'Requires one parameter' >&2
    return 1
  fi

  local action="$1"

  if [ "$action" != start -a "$action" != stop ]
  then
    printf 'Unknown command "%s"\n' "$action" >&2
    return 1
  fi

  su --command "/usr/pgsql-9.3/bin/pg_ctl -w '$action'" --login postgres
}


set -e

main "$@"
