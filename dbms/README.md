# CyVerse DS DBMS Playbooks

This is a collection of playbooks for maintaining the DBMS for the Data Store.

## Tags

* `no_testing` for tasks that shouldn't run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent


## Variables

Variable                    | Required | Default  | Comments
--------------------------- | -------- | -------- | --------
`dbms_irods_password`       | yes      |          | the password for authenticating `dbms_irods_user`
`dbms_irods_user`           | no       | irods    | the user iRODS uses to connect to the ICAT DB
`dbms_log_min_duration`     | no       | 1000     | the number of milliseconds a query should take before it is logged in the DBMS logs. `-1` disables query logging
`dbms_port`                 | no       | 5432     | the TCP port used by the DBMS
`dbms_replication_password` | maybe*   |          | the password for authenticating `dbms_replication_user`, *this is required if replication is being set up
`dbms_replication_start`    | no       | false    | whether or not the role should start replication. WARNING: THIS WILL DESTROY THE CURRENT REPLICA
`dbms_replication_user`     | no       | postgres | the DBMS user authorized to replicate the master node
