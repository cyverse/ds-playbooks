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

SET SESSION work_mem = '1GB';
SET SESSION maintenance_work_mem = '32GB';
SET SESSION max_parallel_maintenance_workers = 15;

\echo creating indexes deferred when adding resc_id to r_data_main
CREATE INDEX CONCURRENTLY idx_data_main1 ON r_data_main (data_id);
CREATE INDEX CONCURRENTLY idx_data_main3 ON r_data_main (coll_id);
CREATE INDEX CONCURRENTLY idx_data_main4 ON r_data_main (data_name);
CREATE INDEX CONCURRENTLY idx_data_main5 ON r_data_main (data_type_name);
CREATE INDEX CONCURRENTLY idx_data_main6 ON r_data_main (data_path);

\echo creating indexes deferred when converting schema to version 6
CREATE INDEX CONCURRENTLY idx_data_main7 ON r_data_main (resc_id);
CREATE INDEX CONCURRENTLY idx_data_main8 ON r_data_main (data_is_dirty);
