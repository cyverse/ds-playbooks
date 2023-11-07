# CyVerse DS DBMS Playbooks

This is a collection of playbooks for maintaining the DBMS for the Data Store.

## Tags

* `no_testing` for tasks that shouldn't run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable                                | Required | Default       | Comments
--------------------------------------- | -------- | ------------- | --------
`dbms_checkpoint_completion_target`     | no       | 0.9           | WAL checkpoint target duration fraction
`dbms_checkpoint_timeout`               | no       | 15            | WAL checkpoint timeout in minutes
`dbms_effective_cache_size`             | no       | _see comment_ | the value the query planner uses to estimate the total size of data caches in GiB, the default in 50% of the total memory
`dbms_effective_io_concurrency`         | no       | 200           | the number of concurrent disk I/O operations that can be executed simultaneously
`dbms_log_line_prefix`                  | no       | < %m %r >     | PostgreSQL log message prefix (see PostgreSQL documentation for possible values)
`dbms_log_min_duration`                 | no       | 1000          | the number of milliseconds a query should take before it is logged in the DBMS logs. `-1` disables query logging
`dbms_maintenance_work_mem`             | no       | 2             | the amount of memory in gibibytes for maintenance operations
`dbms_max_connections`                  | no       | 1500          | the maximum number of connections allowed to the DBMS (change requires restart)
`dbms_max_wal_senders`                  | no       | 120           | the maximum number of WAL sender processes (change requires restart)
`dbms_max_wal_size`                     | no       | 8             | the maximum size of a WAL file in gibibytes
`dbms_max_worker_processes`             | no       | _see comment_ | the maximum number of concurrent worker processes, default is the number of processors (change requires restart)
`dbms_max_parallel_maintenance_workers` | no       | 2*            | the maximum number of parallel processes per maintenance operations, *must be no larger than `max_worker_processes`, so if that is 1, then the default is 1
`dbms_max_parallel_workers_per_gather`  | no       | 2*            | the maximum number of parallel processes that can be started by a single gather or gather merge, *must be no larger than `max_worker_processes`, so if that is 1, then the default is 1
`dbms_mem_num_huge_pages`               | no       | 60000         | the number of huge memory pages supported by the DBMS
`dbms_min_wal_size`                     | no       | 2             | the minimum size of a WAL file in gibibytes
`dbms_port`                             | no       | 5432          | the TCP port used by the DBMS (change requires restart)
`dbms_random_page_cost`                 | no       | 1.1           | the query planning cost of a random page retrieval relative to other costs
`dbms_replication_password`             | maybe*   |               | the password for authenticating `dbms_replication_user`, *this is required if replication is being set up
`dbms_replication_start`                | no       | false         | whether or not the role should start replication. WARNING: THIS WILL DESTROY THE CURRENT REPLICA
`dbms_replication_user`                 | no       | postgres      | the DBMS user authorized to replicate the master node
`dbms_restart_allowed`                  | no       | false         | whether or not the playbooks are allowed to reboot and restart PostgreSQL
`dbms_wal_keep_segments`                | no       | 4000          | the number of WAL files held by the primary server for its replica servers
`dbms_work_mem`                         | no       | 32            | the allowed memory in mebibytes for each sort and hash operation
