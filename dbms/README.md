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
`dbms_replication_password` | yes      |          | the password for authenticating `dbms_replication_user`
`dbms_replication_user`     | no       | postgres | the DBMS user authorized to replicate the master node
