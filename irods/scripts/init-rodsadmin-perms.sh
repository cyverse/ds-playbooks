#!/bin/bash
#
# This script initializes the iRODS permissions for the rodsadmin group.
# rodsadmin will be give write permission on /. For collections and data objects
# in / or any of its immediate member collections, rodsadmin group will be given
# write permission. For everything else, rodsadmin group will be given own
# permission.
#
# This script is intended to be executed on an IES by the service account.
#
# Usage:
#  init-rodsadmin-perms.sh DBMS_HOST DBMS_PORT DB_USER
#
# Parameters:
#  DBMS_HOST  The domain name of the ICAT DBMS server
#  DBMS_PORT  The TCP port on DBMS_HOST where the DBMS listens
#  DB_USER    The DB user to used to connect to the ICAT
#
# Returns:
#  It writes 'true' to standard output if at least one permission was changed,
#  otherwise it writes 'false'.
#
# XXX: Since this needs to run on bash version 4.1+, the script can't rely on
# 'shopt -s lastpipe' to propagate errors from gather_changes. When none of
# our iRODS IES run on CentOS 6, this script should be modified to use this
# instead of relying on the stdout of gather_changes to pass up the exit status.

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

  if [ "$#" -lt 3 ]
  then
    printf 'requires three input parameters\n' >&2
    return 1
  fi

  local dbmsHost="$1"
  local dbmsPort="$2"
  local dbUser="$3"

  set_permissions < <(gather_changes "$dbmsHost" "$dbmsPort" "$dbUser")
}


gather_changes()
{
  local host="$1"
  local port="$2"
  local user="$3"

  psql --no-align --quiet --record-separator-zero --tuples-only --host "$host" --port "$port" \
       ICAT "$user" \
    <<'SQL'
BEGIN;

CREATE TEMPORARY TABLE rodsadmin_perms (object_id, perm) AS
SELECT a.object_id, t.token_name
  FROM r_objt_access AS a JOIN r_tokn_main AS t ON t.token_id = a.access_type_id
  WHERE a.user_id = (SELECT user_id FROM r_user_main WHERE user_name = 'rodsadmin')
    AND t.token_namespace = 'access_type';

CREATE INDEX rodsadmin_perms_idx ON rodsadmin_perms (object_id, perm);

CREATE TEMPORARY TABLE all_entities (id, path, perm) AS
SELECT
    coll_id,
    coll_name,
    CASE WHEN parent_coll_name ~ '^/[^/]*$' THEN 'modify object' ELSE 'own' END
  FROM r_coll_main
  WHERE coll_type != 'linkPoint'
UNION ALL SELECT DISTINCT
    d.data_id,
    c.coll_name || '/' || d.data_name,
    CASE WHEN c.coll_name ~ '^[^/]*$' THEN 'modify object' ELSE 'own' END
  FROM r_coll_main AS c JOIN r_data_main AS d ON d.coll_id = c.coll_id;

CREATE INDEX all_entites_id_idx ON all_entities(id, perm);

SELECT a.perm, a.path
  FROM all_entities AS a
  WHERE NOT EXISTS (
    SELECT r.object_id FROM rodsadmin_perms AS r WHERE r.object_id = a.id AND r.perm = a.perm);

ROLLBACK;
SQL

  local rc="$?"
  printf '%s' "$rc"
}


set_permissions()
{
  local ec=0

  local perm
  local path
  while IFS=\| read -r -d '' perm path || {  ec="$perm" && break; }
  do
    ichmod -M "${perm/modify object/write}" rodsadmin "$path"
    ec="$?"
    ChangedSomething=true
  done

  return "$ec"
}


set -u
main "$@"
