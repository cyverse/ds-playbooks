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
`aegis_base_collection`                    |                                      |         | the base collection for the Aegis project
`aegis_resource_hierarchy`                 | `irods_resource_hierarchies[0]`      |         | the resource used by the Aegis project
`aegis_storage_resource`                   |                                      |         | an Aegis storage resource hosted on the server being configured, _see below_
`avra_base_collection`                     |                                      |         | The base collection for the Avra project. If it isn't present no Avra rules will fire.
`avra_manager`                             | `irods_clerver_user`                 |         | The iRODS user who is responsible for Avra data.
`avra_resource_hierarchy`                  | `irods_resource_hierarchies[0]`      |         | The resource used by the Avra project
`become_svc_acnt`                          | true                                 |         | whether or not to perform actions normally performed by the service account as the service account
`bisque_irods_host`                        | `canonical_hostname`                 |         | The iRODS host to report to BisQue.
`bisque_password`                          | admin                                |         | The password used to authenticate connections to BisQue
`bisque_projects`                          | `[]`                                 |         | A list of projects that automatically publish to BisQue
`bisque_url`                               |                                      |         | The URL for the BisQue server to connect to
`bisque_user`                              | admin                                |         | The user to connect to BisQue as
`build_dir`                                | /tmp                                 |         | The directory used for building artifacts for deployment
`canonical_hostname`                       | `groups['irods_catalog'][0]`         |         | The external FQDN used to access the data store services
`canonical_irods_port`                     | 1247                                 |         | The port on the `canonical_hostname` host listening for connections to iRODS
`captcn_owners`                            | `[]`                                 |         | a list of users who get ownership of CAP_TCN collections
`captcn_readers`                           | `[]`                                 |         | a list of users who get read access to CAP_TCN collections
`captcn_writers`                           | `[]`                                 |         | a list of users who get write access to CAP_TCN collections
`cereus_collections`                       | `[]`                                 |         | a list of collections whose data belongs on the Cereus resource, each entry must be an absolute path
`cereus_resource_hierarchy`                | `irods_resource_hierarchies[0]`      |         | the Cereus resource used for hosting data for Cereus related projects
`check_routes_timeout`                     | 3                                    |         | the number of seconds the `check_route` playbook will wait for a response during a single port check
`de_job_irods_user`                        |                                      |         | The iRODS username used by the DE from running jobs. If undefined, it won't be created.
`firewall_chain`                           | INPUT                                |         | The iptables chain managing authorizing iRODS connections
`irods_admin_username`                     | rods                                 |         | the iRODS admin account name
`irods_amqp_exchange`                      | irods                                |         | the AMQP exchange used to publish events
`irods_amqp_host`                          | `groups['amqp'][0]`                  |         | the FQDN or IP address of the server hosting the AMQP service
`irods_amqp_mgmt_port`                     | 15672                                |         | The TCP port used for management of the AMQP vhost
`irods_amqp_password`                      | guest                                |         | The password iRODS uses to connect to the AMQP vhost
`irods_amqp_port`                          | 5672                                 |         | The TCP port the RabbitMQ broker listens on
`irods_amqp_user`                          | guest                                |         | The user iRODS uses to connect to the AMQP vhost
`irods_ampq_vhost`                         | /                                    |         | The AMQP vhost iRODS connects to
`irods_allowed_clients`                    | 0.0.0.0/0                            |         | The network/mask for the clients allowed to access iRODS.
`irods_clerver_password`                   | rods                                 |         | The password used to authenticate the clerver
`irods_clerver_user`                       | `irods_admin_username`               |         | the rodsadmin user to be used by the server being configured
`irods_db_password`                        | testpassword                         |         | The password iRODS uses when connecting to the ICAT DB.
`irods_db_user`                            | irods                                |         | The user iRODS uses when connecting to the ICAT DB.
`irods_dbms_pg_hba`                        | /etc/postgresql/12/main/pg_hba.conf  |         | The absolute path to the pg_hba.conf file on the DBMS hosting the ICAT DB
`irods_dbms_pg_version`                    | 12                                   | 9.3, 12 | The version of the PostgreSQL client to install.
`irods_dbms_port`                          | 5432                                 |         | The TCP port the DBMS listens on.
`irods_default_dir_mode`                   | 0750                                 |         | the default permissions assigned to newly created directories in the vault
`irods_default_file_mode`                  | 0600                                 |         | the default permissions assigned to newly created files in the vault
`irods_default_number_of_transfer_threads` | 4                                    |         | the default maximum number of transfer streams for parallel transfer
`irods_default_repl_resource`              | `irods_default_resource`             |         | the default resource for replication
`irods_default_resource`                   | `irods_resource_hierarchies[0].name` |         | the name of the default resource
`irods_default_vault`                      |                                      |         | the default path to the vault on the server being configured
`irods_federation`                         | `[]`                                 |         | a list of other iRODS zones to federate with, _see below_
`irods_host_aliases`                       | `[]`                                 |         | a list of other names and addresses used to refer to the host being configured.
`irods_max_num_re_procs`                   | 4                                    |         | the maximum number of rule engine processes to run
`irods_negotiation_key`                    | TEMPORARY_32byte_negotiation_key     |         | the negotiation key
`irods_other_host_entries`                 | []                                   |         | a list of other FQDNs to add to /etc/hosts
`irods_parallel_transfer_buffer_size`      | 4                                    |         | the transfer buffer size in MiB for each stream during parallel transfer
`irods_publish_rs_image`                   | false                                |         | Whether or not to publish a freshly build resource server docker image to dockerhub.
`irods_re_host`                            | `group_vars['irods_catalog'][0]`     |         | the FQDN or IP address of the iRODS rule engine host
`irods_resource_hierarchies`               | `[ { "name": "demoResc" } ]`         |         | the list of resource hierarchies that need to exist, _see below_
`irods_rs_image`                           | ds-irods-rs-onbuild                  |         | the name of the unpublished RS image to be generated
`irods_server_control_plane_key`           | TEMPORARY__32byte_ctrl_plane_key     |         | the server control plane key
`irods_server_port_range_end`              | 20199                                |         | the last address in the range of auxillary TCP and UDP ports
`irods_server_port_range_start`            | 20000                                |         | the first address in the range of auxillary TCP and UDP ports
`irods_service_account_name`               | irods                                |         | the system account used to run the iRODS server processes
`irods_service_group_name`                 | `irods_service_account_name`         |         | the system group used to run the iRODS server processes
`irods_storage_resources`                  | `[]`                                 |         | a list of storage resources hosted on the server being configured, _see below_
`irods_user_password_salt`                 |                                      |         | the salt used when obfuscating user passwords stored in the catalog database
`irods_version`                            | 4.2.8                                |         | the version of iRODS to work with
`irods_zone_key`                           | TEMPORARY_zone_key                   |         | the zone key
`irods_zone_name`                          | tempZone                             |         | the name of the zone
`pire_manager`                             | null                                 |         | The username that owns the PIRE project collection, if `null`, the collection isn't created.
`pire_resource_hierarchy`                  | `irods_resource_hierarchies[0]`      |         | The resource used by the PIRE project
`report_email_addr`                        | root@localhost                       |         | The address where reports are to be emailed.
`restart_irods`                            | `false`                              |         | iRODS can be restarted on the servers having config file changes, _see below_
`sernec_owners`                            | `[]`                                 |         | a list of users who get ownership of sernec collections
`sernec_readers`                           | `[]`                                 |         | a list of users who get read access to sernec collections
`sernec_writers`                           | `[]`                                 |         | a list of users who get write access to sernec collections
`sparcd_admin`                             | null                                 |         | The user name of the Sparc'd administrator. If this isn't set, no sparcd rules will fire.
`sparcd_base_collection`                   | _see description_                    |         | The base iRODS collection used by Sparc'd. If `sparcd_admin` is `null`, the default is `null`, otherwise it is `/{{ irods_zone_name }}/home/{{ sparcd_admin }}/Sparcd/Collections`.
`sparcd_report_email_addr`                 | _see description_                    |         | The email address where SPARC'd notifications are sent. If `sparcd_admin` is `null`, the default is `null`, otherwise it is `report_email_addr`.
`sysctl_kernel`                            | `[]`                                 |         | a list of sysctl kernel parameters to set on the iRODS catalog service provider, _see_below_
`terraref_base_collection`                 |                                      |         | The base collection for the TerraREF project. If it isn't present no TerraREF rules will fire.
`terraref_manager`                         | `irods_clerver_user`                 |         | The iRODS user who is responsible for TerraREF data.
`terraref_resource_hierarchy`              | `irods_resource_hierarchies[0]`      |         | The resource used by the TerraREF project.
`webdav_allowed_src`                       | `[ "0.0.0.0/0" ]`                    |         | A list of network/masks for the clients allowed direct access to the WebDAV servers
`webdav_auth_name`                         | CyVerse                              |         | Authorization realm to use for the Data Store
`webdav_cache_dir`                         | `/var/cache/varnish`                 |         | The directory varnish-cache will use for the WebDAV cache
`webdav_cache_size`                        | 1000                                 |         | The maximum size in mebibytes the cache can be
`webdav_cache_max_file_size`               | 10                                   |         | The maximum size in mebibytes of the largest WebDAV file varnish-cache will cache
`webdav_cache_ttl_fraction`                | 0.1                                  |         | The fraction elapsed time since the last-modified time of a file for cache TTL (Time-to-live) configuration
`webdav_cache_max_ttl`                     | 86400                                |         | The maximum cache TTL in seconds
`webdav_server_limit`                      | 48                                   |         | the number of cpu cores to be used
`webdav_threads_per_child`                 | 4                                    |         | the number of threads per core to be created
`webdav_max_request_workers`               | 192                                  |         | the upper limit on the number of simultaneous requests that will be served. This typically have the value of `webdav_server_limit` multiplied by `webdav_threads_per_child`
`webdav_access_limit`                      | 120                                  |         | The upper limit on the number of simultaneous requests that will be served by webdav
`webdav_davrods_access_limit`              | 80                                   |         | The upper limit on the number of simultaneous requests that will be served by davrods
`webdav_tls_cert_file`                     | `/etc/ssl/certs/dummy.crt`           |         | The TLS certificate file used for encrypted communication
`weddav_tls_chain_file`                    | `/etc/ssl/certs/dummy-chain.crt`     |         | The TLS certificate chain file used for encrypted communication
`webdav_tls_key_file`                      | `/etc/ssl/certs/dummy.key`           |         | The TLS key file used for encrypted communication
`webdav_varnish_service_port`              | 6081                                 |         | The service port number for varnish-cache


The `restart_irods` flag is ignored in the `main.yml` playbook.

`irods_federatation` entry fields

All of them are required.

Field                    | Comments
------------------------ | --------
`catalog_provider_hosts` | a list of the catalog service providers in the federate, each indicated by its FQDN or IP address
`negotiation_key`        | the 32-byte encryption key of the federate
`zone_key`               | the shared authentication secret of the federate
`zone_name`              | the name of the federated zone

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
