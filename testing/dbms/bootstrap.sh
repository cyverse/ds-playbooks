#! /bin/bash

set -e


terminate()
{
  /usr/pgsql-9.3/bin/pg_ctl -w stop
  exit "$?"
}


/usr/pgsql-9.3/bin/pg_ctl -w start
trap 'kill ${!}; terminate' SIGTERM

while true
do
  tail --follow /dev/null &
  wait ${!}
done
