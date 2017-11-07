#! /bin/bash

set -e

printf 'Starting postgresql: '

if err=$(/usr/pgsql-9.3/bin/pg_ctl -w start 2>&1 > /dev/null)
then
  printf 'SUCCESS\n'
  
  exec bash
else
  printf 'FAILURE\n'
  printf '%s\n' "$err"
  exit 1
fi
