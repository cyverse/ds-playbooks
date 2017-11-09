#! /bin/bash

set -e

printf 'Starting postgresql\n'

if ! err=$(/usr/pgsql-9.3/bin/pg_ctl -w start 2>&1)
then
  printf '%s\n' "$err" >&2
  exit 1
fi

printf 'done\n'
exec bash
