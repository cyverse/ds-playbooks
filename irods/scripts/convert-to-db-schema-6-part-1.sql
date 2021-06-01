-- When combined with convert-to-db-schema-6-part-2.sql, this is a pure SQL
-- version of the portions of
-- /var/lib/irods/scripts/irods/database_upgrade.py that upgrade the schema to
-- version 6, optimized for upgrading the ICAT DB in the CyVerse iRODS 4.1.10
-- deployment to iRODS 4.2.8. The script can can be found at
-- https://github.com/irods/irods/blob/4.2.8/scripts/irods/database_upgrade.py.

-- Prior to running this script add the following to
-- /etc/postgres/12/main/conf.d/cyverse.conf, and restart postgres. After
-- running the script, remove them, and restart postgres again.
--
--     max_wal_senders = 0
--     wal_level = minimal
--     work_mem = 2GB
--     maintenance_work_mem = 64GB
--     max_parallel_maintenance_workers = 30
--
-- After this script completes, convert-to-db-schema-6-part-2.sql should be run.
-- `irodsctl start` can be executed at the same time as that SQL script.

\timing on

-- Our database schema  version is 4, so the conversion steps for versions 2 - 4
-- can be mostly skipped. We are mssing a specific query that was supposed to be
-- added in version 3. The follow statement will add that query.

\echo adding missing the DataObjInCollReCur specific query.
INSERT INTO r_specific_query (alias, sqlStr, create_ts)
VALUES (
	'DataObjInCollReCur',
	'WITH coll AS (SELECT coll_id, coll_name FROM r_coll_main WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_hier FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num',
	-- XXX: create_ts should begin with 0 for consistency with its other values
	/*'1388534400'*/ '01388534400' );

\echo
\echo begin conversion to schema version 6
\echo

\echo creating and populating r_data_main.resc_id
-- For each entry in r_data_main set resc_id by using r_resc_hier to look up the
-- Id of the corresponding storage resource in r_resc_main. Here's a pseudocode
-- summary of the logic in database_upgrade.py.
--
--   for ($resc_id, $resc_name) in (SELECT resc_id, resc_name FROM r_resc_main):
--     UPDATE r_data_main
--     SET resc_id = $resc_id::BIGINT
--     WHERE resc_hier = $resc_name OR resc_hier LIKE '%;' || $resc_name;
--
-- Only storage resources will have Ids set in r_data_main. Since storage
-- resources have no child resources, `r_resc_main.resc_children = ''`. We don't
-- use the bundleResc resource, so we can exclude it. Also, all of our storage
-- resources have a parent resource, meaning its resource hierarchy value will
-- begin with the parent name followed by a semicolon (';') and end with the
-- storage resource name. This means we can exclude the condition
-- `resc_hier = $resc_name`. Here's a rewrite in SQL statement of the above
-- pseudocode with these simplification as a single SQL statement. The following
-- post describes an optimized way to and add a column to a large table:
-- https://dba.stackexchange.com/questions/52517/best-way-to-populate-a-new-column-in-a-large-table.
BEGIN;

CREATE TEMPORARY TABLE hierarchies(id, hier) ON COMMIT DROP AS
WITH RECURSIVE hier_steps AS (
	SELECT resc_id AS id, '' || resc_name AS hier, resc_parent AS parent
	FROM r_resc_main
	WHERE resc_children = ''
	UNION
	SELECT c.id, p.resc_name || ';' || c.hier, p.resc_parent
	FROM r_resc_main AS p JOIN hier_steps AS c ON c.parent = p.resc_name )
SELECT id, hier FROM hier_steps WHERE parent = '';

CREATE INDEX idx_hierarchies ON hierarchies(hier, id);

LOCK TABLE r_data_main IN SHARE MODE;

SET LOCAL work_mem = '32GB';

CREATE TABLE r_data_main_new AS
  SELECT d.*, r.id AS resc_id
  FROM r_data_main AS d LEFT JOIN hierarchies AS r ON r.hier = d.resc_hier;

ALTER TABLE r_data_main_new
	ALTER COLUMN data_id SET NOT NULL,
	ALTER COLUMN coll_id SET NOT NULL,
	ALTER COLUMN data_name SET NOT NULL,
	ALTER COLUMN data_version SET DEFAULT '0'::CHARACTER VARYING,
	ALTER COLUMN data_type_name SET NOT NULL,
	ALTER COLUMN data_size SET NOT NULL,
	ALTER COLUMN resc_name SET NOT NULL,
	ALTER COLUMN data_path SET NOT NULL,
	ALTER COLUMN data_owner_name SET NOT NULL,
	ALTER COLUMN data_owner_zone SET NOT NULL,
	ALTER COLUMN data_is_dirty SET DEFAULT 0,
	ALTER COLUMN data_map_id SET DEFAULT 0;

DROP TABLE r_data_main;
ALTER TABLE r_data_main_new RENAME TO r_data_main;

CREATE UNIQUE INDEX idx_data_main2 ON r_data_main (coll_id, data_name, data_repl_num, data_version);

\echo
\echo deferring creation of remain indexes to another script so they can be done
\echo in parallel with rest of upgrade
\echo

COMMIT;

\echo adding resc_parent_context column to r_resc_main
ALTER TABLE r_resc_main ADD resc_parent_context VARCHAR(4000);

\echo changing definition of the DataObjInCollReCur specific query
-- It will use r_data_main.resc_id to identify the storage resource instead of
-- the defunct r_data_main.resc_hier column.
UPDATE r_specific_query
SET sqlstr =
	'WITH coll AS (SELECT coll_id, coll_name FROM R_COLL_MAIN WHERE R_COLL_MAIN.coll_name = ? OR R_COLL_MAIN.coll_name LIKE ?) SELECT DISTINCT d.data_id, (SELECT coll_name FROM coll WHERE coll.coll_id = d.coll_id) coll_name, d.data_name, d.data_repl_num, d.resc_name, d.data_path, d.resc_id FROM R_DATA_MAIN d WHERE d.coll_id = ANY(ARRAY(SELECT coll_id FROM coll)) ORDER BY coll_name, d.data_name, d.data_repl_num'
WHERE alias = 'DataObjInCollReCur';

\echo repurposing r_resc_main.resc_parent as a foreign key to r_resc_main.resc_id
UPDATE r_resc_main AS rdm
SET resc_parent = am.resc_id
FROM (SELECT resc_name, resc_id FROM r_resc_main) AS am
WHERE am.resc_name = rdm.resc_parent;

\echo populating r_resc_main.resc_parent_context
-- For each child resource in r_resc_main, set the value of resc_parent_context
-- from its value in the parent resource's resc_children entry. Here's a
-- pseudocode summary of the logic in database_upgrade.py
--
--   for ($resc_id, $resc_children) in (
--     SELECT resc_id, resc_children FROM r_resc_main WHERE resc_children IS NOT NULL
--   ):
--     # resc_children has the form '[<resc-child>[;<resc-child>]*]' where
--     # <resc-child> has the form '<child-name>{[<parent-context>]}'. The
--     # following statement creates a <resc-child> list by splitting
--     # resc_children on ';'. It then tranforms this list into a list of
--     # <child-name>*<parent-context> tuples.
--     $child_contexts = [
--       (m.group(1), m.group(2)) for m in [
--         '^([^{}]*)\\{([^{}]*)\\}'.match(s) for s in $resc_children.split(';')
--       ] if m ]
--     for ($child_name, $context) in $child_contexts:
--       UPDATE r_resc_main SET resc_parent_context = $context WHERE resc_name = $child_name
--
-- iRODS sets r_resc_main.resc_children to an empty string ('') instead of NULL
-- when the resource has no chidren. Given this, here's a rewrite of the above
-- pseudocode as a single SQL statement.
UPDATE r_resc_main AS cr
SET resc_parent_context = pr.child_context[2]
FROM (
		SELECT
			resc_id,
			REGEXP_MATCH(REGEXP_SPLIT_TO_TABLE(resc_children, ';'), E'^([^{}]*)\{([^{}]*)\}')
				AS child_context
		FROM r_resc_main
		WHERE resc_children != ''
	) AS pr
WHERE cr.resc_name = pr.child_context[1];

\echo
\echo deferring creation of idx_data_main7 and idx_data_main8 to another script
\echo so it can be done in parallel with rest of upgrade
\echo

\echo completing conversion to schema version 6
UPDATE r_grid_configuration
SET option_value = 6
WHERE namespace = 'database' AND option_name = 'schema_version';
