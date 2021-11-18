#!/bin/bash
#
# This script initializes the iRODS permissions for the rodsadmin group.
# rodsadmin will be give write permission on /. For collections and data objects
# in / or any of its immediate member collections, rodsadmin group will be given
# write permission. For everything else, rodsadmin group will be given own
# permission.
#
# This script is intended to be executed on a catalog service provider by the 
# service account.
#
# Usage:
#  init-rodsadmin-perms.sh DBMS_HOST DBMS_PORT DB_USER
#
# Parameters:
#  DBMS_HOST  The domain name of the ICAT DBMS server
#  DBMS_PORT  The TCP port on DBMS_HOST where the DBMS listens
#  DB_USER    The DB user to used to connect to the ICAT
#  ZONE       The iRODS zone being modified
#
# Returns:
#  It writes 'true' to standard output if at least one permission was changed,
#  otherwise it writes 'false'.

shopt -s lastpipe
set -o errexit -o nounset -o pipefail

readonly CHANGES=$(mktemp)


main()
{
  trap finish_up EXIT

  if [ "$#" -lt 9 ]
  then
    printf 'requires nine input parameters\n' >&2
    return 1
  fi

  local dbmsHost="$1"
  local dbmsSvcUser="$2"
  local sshDbmsPort="$3"
  local sshDbmsUser="$4"
  local irodsHost="$5"
  local irodsSvcUser="$6"
  local sshIrodsPort="$7"
  local sshIrodsUser="$8"
  local zone="$9"

  gather_changes "$dbmsHost" "$sshDbmsPort" "$sshDbmsUser" "$dbmsSvcUser" "$zone" > "$CHANGES"

  if [ -s "$CHANGES" ]
  then
    set_permissions "$irodsHost" "$sshIrodsPort" "$sshIrodsUser" "$irodsSvcUser" write < "$CHANGES"
    set_permissions "$irodsHost" "$sshIrodsPort" "$sshIrodsUser" "$irodsSvcUser" own < "$CHANGES"
    printf true
  else
    printf false
  fi
}


finish_up()
{
  local exitCode="$?"

  rm --force "$CHANGES"
  exit "$exitCode"
}


extract_path()
{
  while IFS=\| read -r -d '' perm path
  do
    printf '%s\x00' "$path"
  done
}


gather_changes()
{
  local dbmsHost="$1"
  local sshDbmsPort="$2"
  local sshDbmsUser="$3"
  local dbmsSvcUser="$4"
  local zone="$5"

  ssh -q -p "$sshDbmsPort" "$sshDbmsUser"@"$dbmsHost" \
      "sudo --login --user='$dbmsSvcUser' \\
        psql --no-align --quiet --record-separator-zero --tuples-only ICAT" \
    <<SQL
BEGIN;

CREATE TEMPORARY TABLE rodsadmin_perms (object_id, perm) AS
SELECT a.object_id, t.token_name
  FROM r_objt_access AS a JOIN r_tokn_main AS t ON t.token_id = a.access_type_id
  WHERE a.user_id = (SELECT user_id FROM r_user_main WHERE user_name = 'rodsadmin')
    AND t.token_namespace = 'access_type';

CREATE INDEX rodsadmin_perms_idx ON rodsadmin_perms (object_id);

CREATE TEMPORARY TABLE all_entities (id, path) AS
SELECT coll_id, coll_name FROM r_coll_main
UNION SELECT d.data_id, c.coll_name || '/' || d.data_name
  FROM r_data_main AS d JOIN r_coll_main AS c ON c.coll_id = d.coll_id;

CREATE INDEX all_entities_idx ON all_entities(id);

CREATE TEMPORARY TABLE all_with_perms (path, actual_perm, expected_perm) AS
SELECT a.path, r.perm, CASE WHEN a.path ~ '^/$zone/[^/]*/.*' THEN 'own' ELSE 'modify object' END
FROM all_entities AS a LEFT JOIN rodsadmin_perms AS r ON r.object_id = a.id
WHERE a.path ~ E'^/($zone(/.*)?)?\$';

SELECT expected_perm, path FROM all_with_perms WHERE actual_perm IS DISTINCT FROM expected_perm;

ROLLBACK;
SQL
}


set_permissions()
{
  local irodsHost="$1"
  local sshIrodsPort="$2"
  local sshIrodsUser="$3"
  local irodsSvcUser="$4"
  local perm="$5"

  local permLabel
  if [ "$perm" = write ]
  then
    permLabel='modify object'
  else
    permLabel=own
  fi

  sed --null-data --quiet '/^'"$permLabel"'/p' \
    | extract_path \
    | ssh -q -p "$sshIrodsPort" "$sshIrodsUser"@"$irodsHost" \
      "sudo --login --user '$irodsSvcUser' \\
        xargs --no-run-if-empty --null ichmod -M "$perm" rodsadmin"
}


main "$@"
