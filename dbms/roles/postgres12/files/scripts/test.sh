#!/bin/bash
set -e
set -u

# for pghostname in $@ ; do
#   whoami
psql -t -c 'select pg_is_in_recovery()'
# done

# "select slot_name from pg_replication_slots"
#
# if not master ; then
#   if not slot ; then
#     make slot;
#   fi
# fi
