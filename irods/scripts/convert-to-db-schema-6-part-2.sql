-- When combined with convert-to-db-schema-6-part-1.sql, this is a pure SQL
-- version of the portions of
-- /var/lib/irods/scripts/irods/database_upgrade.py that upgrade the schema to
-- version 6, optimized for upgrading the ICAT DB in the CyVerse iRODS 4.1.10
-- deployment to iRODS 4.2.8. The script can can be found at
-- https://github.com/irods/irods/blob/4.2.8/scripts/irods/database_upgrade.py.
--
-- Prior to running this script, convert-to-db-schema-6-part-1.sql should be
-- run. Once that script completes, this one can be run. `irodsct start` can be
-- executed at the same time as this script.

\timing on

SET SESSION work_mem = '2GB';
SET SESSION maintenance_work_mem = '32GB';
SET SESSION max_parallel_workers = 16;
SET SESSION max_parallel_maintenance_workers = 16;

\echo
\echo creating indexes deferred when adding resc_id to r_data_main
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main1 ON r_data_main (data_id);
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main3 ON r_data_main (coll_id);
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main4 ON r_data_main (data_name);
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main5 ON r_data_main (data_type_name);
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main6 ON r_data_main (data_path);

\echo
\echo creating indexes deferred when converting schema to version 6
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main7 ON r_data_main (resc_id);
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_data_main8 ON r_data_main (data_is_dirty);

\echo
\echo creating custom indexes for DE

-- this allows index-only scans when filtering the table by name, name+value,
-- or name+value+unit, and allows returning any or all of the three plus
-- meta_id (useful for then further joining to r_objt_metamap)
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_meta_main_ipc1
ON r_meta_main (meta_attr_name, meta_attr_value, meta_attr_unit)
INCLUDE (meta_id);

-- this index will allow queries that join on the object_id to both filter by
-- user_id and to return both user_id and access_type_id. Therefore, a join that
-- collects access levels for a given object will be able to use an index-only
-- scan
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_objt_access_ipc1
ON r_objt_access (object_id, user_id)
INCLUDE (access_type_id);

-- this is the same as an index already on r_objt_metamap, but with the fields
-- reversed. This allows joins on either foreign key to productively use an
-- index (and either way, return the other one in an index-only scan) – this
-- should allow almost everything using the table to benefit from index-only
-- scans.
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_objt_metamap_ipc1
ON r_objt_metamap (meta_id, object_id);

--this allows index-only scans joining on user_id to return the user and zone
-- names, which can be useful when returning usernames when listing access
-- permissions for an object
CREATE INDEX CONCURRENTLY IF NOT EXISTS idx_user_main_ipc1
ON r_user_main (user_id)
INCLUDE (user_name, zone_name);
