# CyVerse DS iRODS Playbooks

This is a collection of playbooks for deploying iRODS for the CyVerse Data Store.


## Tags

* `firewall` for tasks related to firewall configuration
* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent


## Variables

None of these variables are required.

Variable                                   | Default                              | Choices | Comments
------------------------------------------ | ------------------------------------ | ------- | --------
`become_svc_acnt`                          | true                                           | whether or not to perform actions normally performed by the service account as the service account
`bisque_irods_host`                        | `irods_ies`                          |         | The iRODS host to report to BisQue.
`bisque_password`                          | admin                                |         | The password used to authenticate connections to BisQue
`bisque_projects`                          | `[]`                                 |         | A list of projects that automatically publish to BisQue
`bisque_url`                               |                                      |         | The URL for the BisQue server to connect to
`bisque_user`                              | admin                                |         | The user to connect to BisQue as
`dbms_password`                            | irods                                |         | The password iRODS uses when connecting to the DBMS hosting the ICAT DB.
`dbms_port`                                | 5432                                 |         | The TCP port the DBMS listens on.
`dbms_username`                            | irods                                |         | The user iRODS uses when connecting to the DBMS hosting the ICAT DB.
`de_job_irods_user`                        |                                      |         | The iRODS username used by the DE from running jobs. If undefined, it won't be created.
`firewall_chain`                           | INPUT                                |         | The iptables chain managing authorizing iRODS connections
`irods_aegis_repl_resource`                | aegisReplRes                         |         | the name of the aegis resource where replicas are written
`irods_aegis_resource`                     | aegisIngestRes                       |         | the name of the aegis resource where newly uploaded files are written
`irods_allowed_clients`                    | 0.0.0.0/0                            |         | The network/mask for the clients allowed to access iRODS.
`irods_clerver_password`                   | rods                                 |         | The password used to authenticate the clerver
`irods_clerver_user`                       | rods                                 |         | the rodsadmin user to be used by the server being configured
`irods_default_dir_mode`                   | 0750                                 |         | the default permissions assigned to newly created directories in the vault
`irods_default_file_mode`                  | 0600                                 |         | the default permissions assigned to newly created files in the vault
`irods_default_number_of_transfer_threads` | 4                                    |         | the default maximum number of transfer streams for parallel transfer
`irods_default_repl_resource`              | `irods_default_resource`             |         | the default resource for replication
`irods_default_resource`                   | `irods_resource_hierarchies[0].name` |         | the name of the default resource
`irods_default_vault`                      |                                      |         | the default path to the vault on the server being configured
`irods_federation`                         | `[]`                                 |         | a list of other iRODS zones to federate with, _see below_
`irods_host_aliases`                       | `[]`                                 |         | a list of other names and addresses used to refer to the host being configured.
`irods_ies`                                | `group_vars['ies'][0]`               |         | the FQDN of the IES
`irods_max_num_re_procs`                   | 4                                    |         | the maximum number of rule engine processes to run
`irods_negotiation_key`                    | TEMPORARY_32byte_negotiation_key     |         | the negotiation key
`irods_other_host_entries`                 | []                                   |         | a list of other FQDNs to add to /etc/hosts
`irods_parallel_transfer_buffer_size`      | 4                                    |         | the transfer buffer size in MiB for each stream during parallel transfer
`irods_resource_hierarchies`               | `[ { "name": "demoResc" } ]`         |         | the list of resource hierarchies that need to exist, _see below_
`irods_server_control_plane_key`           | TEMPORARY__32byte_ctrl_plane_key     |         | the server control plane key
`irods_server_port_range_end`              | 20199                                |         | the last address in the range of auxillary TCP and UDP ports
`irods_server_port_range_start`            | 20000                                |         | the first address in the range of auxillary TCP and UDP ports
`irods_service_account_name`               | irods                                |         | the system account used to run the iRODS server processes
`irods_service_group_name`                 | `irods_service_account_name`         |         | the system group used to run the iRODS server processes
`irods_storage_resources`                  | `[]`                                 |         | a list of storage resources hosted on the server being configured, _see below_
`irods_version`                            | 4.1.11                               |         | the version of iRODS to work with
`irods_zone_key`                           | TEMPORARY_zone_key                   |         | the zone key
`irods_zone_name`                          | tempZone                             |         | the name of the zone
`load_balancer_irods_allowed_src`          | 0.0.0.0/0                            |         | The network/mask for the clients allowed to access iRODS through the load balancer.
`load_balancer_irods_max_conn`             | 100                                  |         | The maximum number of concurrent connections to iRODS through the load balancer.
`load_balancer_irods_proxy_port`           | 1247                                 |         | The port the load balancer will listen on for connections to iRODS.
`load_balancer_stats_allowed_src`          | 0.0.0.0/0                            |         | The network/mask for hosts allowed to see the HAProxy stats web page.
`load_balancer_stats_certificate`          | /etc/haproxy/certs/stats.pem         |         | The TLS certificate used by the stats endpoint
`load_balancer_stats_port`                 | 81                                   |         | The TCP port used to retrieve HAProxy stats
`load_balancer_stats_user`                 | haproxy-stats                        |         | The user to authenticate as to access the stats service
`load_balancer_stats_password`             | B1Gp4sSw0rD!!                        |         | The password used to authetnicate access to the stats service
`pire_quota`                               | 0                                    |         | The quota for the PIRE project. A `0` means no quota.
`pire_resource_hierarchy`                  | `irods_resource_hierarchies[0]`      |         | The resource server used by the PIRE project
`rabbitmq_ephemeral`                       | `true`                               |         | whether or not the `irods` AMQP exchange will persist when iRODS disconnects from the AMQP broker
`rabbitmq_password`                        | guest                                |         | The password iRODS uses to connect to the AMQP vhost
`rabbitmq_port`                            | 5672                                 |         | The TCP port the RabbitMQ broker listens on
`rabbitmq_user`                            | guest                                |         | The user iRODS uses to connect to the AMQP vhost
`rabbitmq_vhost`                           | /                                    |         | The AMQP vhost iRODS connects to
`restart_irods`                            | `false`                              |         | iRODS can be restarted on the servers having config file changes, _see below_
`sanimal_irods_base_coll`                  |                                      |         | The base iRODS collection used by Sanimal. If this isn't set, no sanimal rules will fire.
`sernec_owners`                            | `[]`                                 |         | a list of users who get ownership of sernec collections
`sernec_readers`                           | `[]`                                 |         | a list of users who get read access to sernec collections
`sernec_writers`                           | `[]`                                 |         | a list of users who get write access to sernec collections
`single_threaded_resources`                | `[]`                                 |         | a list of resources that only support single threaded transfers
`sysctl_kernel`                            | `[]`                                 |         | a list of sysctl kernel parameters to set for the IES, _see_below_
`webdav_host`                              | localhost                            |         | The FQDN or IP address of the WebDAV host

The `restart_irods` flag is ignored in the `main.yml` playbook.

`irods_federatation` entry fields

All of them are required.

Field             | Comments
----------------- | --------
`icat_host`       | the hostname of the IES in the federate
`negotiation_key` | the 32-byte encryption key of the federate
`zone_key`        | the shared authentication secret of the federate
`zone_name`       | the name of the federated zone

`irods_resource_hierarchies` entry fields

Field      | Required | Default | Comments
---------- | -------- | ------- | --------
`children` | no       | `[]`    | A list of child hierarchy definitions have the same form as an `irods_resource_hierarchies` entry
`context`  | no       |         | A context to attach to this resource
`name`     | yes      |         | The name of the resource
`type`     | no       |         | For a coordinating resource, this is the type of resource. For a storage resource this should not be provided.

`irods_storage_resources` entry fields

All of them are required.

Field     | Comments
--------- | --------
`context` | The context to assign to the resource
`name`    | The name of the storage resource
`vault`   | The absolute path to the vault hold the files on this resource.

`sysctl_kernel` entry fields

Both of them are required.

Field    | Comments
-------- | --------
`name`   | The parameter name to modify
`value`  | The new value to set
