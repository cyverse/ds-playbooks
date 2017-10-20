CyVerse DS iRODS Playbooks
==========================

This is a collection of playbooks for deploying iRODS for the CyVerse Data Store.

Variables
--------------

None of these variables are required.

Variable                             | Default                          | Choices | Comments
------------------------------------ | -------------------------------- | ------- | --------
`bisque_irods_host`                  | `icat_host`                      |         | The iRODS host to report to BisQue.
`dbms_password`                      | irods                            |         | The password iRODS uses when connecting to the DBMS hosting the ICAT DB.
`dbms_port`                          | 5432                             |         | The TCP port the DBMS listens on.
`dbms_username`                      | irods                            |         | The user iRODS uses when connecting to the DBMS hosting the ICAT DB.
`default_number_of_transfer_threads` | 4                                |         | the default maximum number of transfer streams for parallel transfer
`ds_federation`                      | []                               |         | a list of other iRODS zones to federate with, _see below_
`host_aliases`                       | []                               |         | a list of other names and addresses used to refer to the host being configured.
`icat_host`                          | `group_vars['ies'][0]`           |         | the FQDN of the IES
`irods_aegis_repl_resource`          | aegisReplRes                     |         | the name of the aegis resource where replicas are written
`irods_aegis_resource`               | aegisIngestRes                   |         | the name of the aegis resource where newly uploaded files are written
`irods_clerver_user`                 | rods                             |         | the rodsadmin user to be used by the server being configured
`irods_default_repl_resource`        | `irods_default_resource`         |         | the default resource for replication
`irods_default_resource`             | demoResc                         |         | the name of the default resource
`irods_max_num_re_procs`             | 4                                |         | the maximum number of rule engine processes to run
`negotiation_key`                    | TEMPORARY_32byte_negotiation_key |         | the negotiation key
`parallel_transfer_buffer_size`      | 4                                |         | the transfer buffer size in MiB for each stream during parallel transfer
`rabbitmq_ephemeral`                 | true                             |         | whether or not the `irods` AMQP exchange will persist when iRODS disconnects from the AMQP broker
`rabbitmq_password`                  | guest                            |         | The password iRODS uses to connect to the AMQP vhost
`rabbitmq_port`                      | 5672                             |         | The TCP port the RabbitMQ broker listens on
`rabbitmq_user`                      | guest                            |         | The user iRODS uses to connect to the AMQP vhost
`rabbitmq_vhost`                     | /                                |         | The AMQP vhost iRODS connects to
`sernec_owners`                      | []                               |         | a list of users who get ownership of sernec collections
`sernec_readers`                     | []                               |         | a list of users who get read access to sernec collections
`sernec_writers`                     | []                               |         | a list of users who get write access to sernec collections
`server_control_plane_key`           | TEMPORARY__32byte_ctrl_plane_key |         | the server control plane key
`server_port_range_end`              | 20199                            |         | the last address in the range of auxillary TCP and UDP ports
`server_port_range_start`            | 20000                            |         | the first address in the range of auxillary TCP and UDP ports
`single_threaded_resources`          | []                               |         | a list of resources that only support single threaded transfers
`zone_key`                           | TEMPORARY_zone_key               |         | the zone key

`irods_federate fields`

All of them are required.

Variable          | Comments
----------------- | --------
`icat_host`       | the hostname of the IES in the federate
`negotiation_key` | the 32-byte encryption key of the federate
`zone_key`        | the shared authentication secret of the federate
`zone_name`       | the name of the federated zone
