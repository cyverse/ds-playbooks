# CyVerse DS iRODS Playbooks

This is a collection of playbooks for deploying iRODS for the CyVerse Data Store.

## Tags

* `firewall` for tasks related to firewall configuration
* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable                                   | Required | Default                              | Choices | Comments
------------------------------------------ | -------- | ------------------------------------ | ------- | --------
`avra_base_collection`                     | no       |                                      |         | The base collection for the Avra project. If it isn't present no Avra rules will fire.
`avra_manager`                             | no       | `irods_clerver_user`                 |         | The iRODS user who is responsible for Avra data.
`avra_resource_hierarchy`                  | no       | `irods_resource_hierarchies[0]`      |         | The resource used by the Avra project
`become_svc_acnt`                          | no       | true                                 |         | Whether or not to perform actions normally performed by the service account as the service account
`bisque_irods_host`                        | no       | `canonical_hostname`                 |         | The iRODS host to report to BisQue.
`bisque_password`                          | no       | admin                                |         | The password used to authenticate connections to BisQue
`bisque_projects`                          | no       | []                                   |         | A list of projects that automatically publish to BisQue
`bisque_url`                               | no       |                                      |         | The URL for the BisQue server to connect to
`bisque_user`                              | no       | admin                                |         | The user to connect to BisQue as
`build_dir`                                | no       | /tmp                                 |         | The directory used for building artifacts for deployment
`canonical_hostname`                       | no       | `groups['irods_catalog'][0]`         |         | The external FQDN used to access the data store services
`canonical_irods_port`                     | no       | 1247                                 |         | The port on the `canonical_hostname` host listening for connections to iRODS
`captcn_owners`                            | no       | []                                   |         | A list of users who get ownership of CAP_TCN collections
`captcn_readers`                           | no       | []                                   |         | A list of users who get read access to CAP_TCN collections
`captcn_writers`                           | no       | []                                   |         | A list of users who get write access to CAP_TCN collections
`cereus_collections`                       | no       | []                                   |         | A list of collections whose data belongs on the Cereus resource, each entry must be an absolute path
`cereus_resource_hierarchy`                | no       | `irods_resource_hierarchies[0]`      |         | the Cereus resource used for hosting data for Cereus related projects
`check_routes_timeout`                     | no       | 3                                    |         | The number of seconds the `check_route` playbook will wait for a response during a single port check
`de_job_irods_user`                        | no       |                                      |         | The iRODS username used by the DE from running jobs. If undefined, it won't be created.
`firewall_chain`                           | no       | INPUT                                |         | The iptables chain managing authorizing iRODS connections
`irods_admin_password`                     | no       | `irods_clerver_password`             |         | The iRODS admin account password
`irods_admin_username`                     | no       | `irods_clerver_user`                 |         | The iRODS admin account name
`irods_amqp_exchange`                      | no       | irods                                |         | The AMQP exchange used to publish events
`irods_amqp_host`                          | no       | `groups['amqp'][0]`                  |         | the FQDN or IP address of the server hosting the AMQP service
`irods_amqp_mgmt_port`                     | no       | 15672                                |         | The TCP port used for management of the AMQP vhost
`irods_amqp_password`                      | no       | guest                                |         | The password iRODS uses to connect to the AMQP vhost
`irods_amqp_port`                          | no       | 5672                                 |         | The TCP port the RabbitMQ broker listens on
`irods_amqp_user`                          | no       | guest                                |         | The user iRODS uses to connect to the AMQP vhost
`irods_ampq_vhost`                         | no       | /                                    |         | The AMQP vhost iRODS connects to
`irods_allowed_clients`                    | no       | 0.0.0.0/0                            |         | The network/mask for the clients allowed to access iRODS.
`irods_clerver_password`                   | no       | rods                                 |         | The password used to authenticate the clerver
`irods_clerver_user`                       | no       | rods                                 |         | the rodsadmin user to be used by the server being configured
`irods_db_password`                        | no       | testpassword                         |         | The password iRODS uses when connecting to the ICAT DB.
`irods_db_user`                            | no       | irods                                |         | The user iRODS uses when connecting to the ICAT DB.
`irods_dbms_pg_hba`                        | no       | /etc/postgresql/12/main/pg_hba.conf  |         | The absolute path to the pg_hba.conf file on the DBMS hosting the ICAT DB
`irods_dbms_pg_version`                    | no       | 12                                   | 9.3, 12 | The version of the PostgreSQL client to install.
`irods_dbms_port`                          | no       | 5432                                 |         | The TCP port the DBMS listens on.
`irods_default_dir_mode`                   | no       | 0750                                 |         | The default permissions assigned to newly created directories in the vault
`irods_default_file_mode`                  | no       | 0600                                 |         | The default permissions assigned to newly created files in the vault
`irods_default_number_of_transfer_threads` | no       | 4                                    |         | The default maximum number of transfer streams for parallel transfer
`irods_default_repl_resource`              | no       | `irods_default_resource`             |         | The default resource for replication
`irods_default_resource`                   | no       | `irods_resource_hierarchies[0].name` |         | the name of the default resource
`irods_default_vault`                      | no       |                                      |         | The default path to the vault on the server being configured
`irods_federation`                         | no       | []                                   |         | A list of other iRODS zones to federate with, _see below_
`irods_host_aliases`                       | no       | []                                   |         | A list of other names and addresses used to refer to the host being configured.
`irods_max_num_re_procs`                   | no       | 4                                    |         | The maximum number of rule engine processes to run
`irods_negotiation_key`                    | no       | TEMPORARY_32byte_negotiation_key     |         | The negotiation key
`irods_other_host_entries`                 | no       | []                                   |         | A list of other FQDNs to add to /etc/hosts
`irods_parallel_transfer_buffer_size`      | no       | 4                                    |         | The transfer buffer size in MiB for each stream during parallel transfer
`irods_publish_rs_image`                   | no       | false                                |         | Whether or not to publish a freshly build resource server docker image to dockerhub.
`irods_re_host`                            | no       | `group_vars['irods_catalog'][0]`     |         | The FQDN or IP address of the iRODS rule engine host
`irods_resource_hierarchies`               | no       | `[ { "name": "demoResc" } ]`         |         | The list of resource hierarchies that need to exist, _see below_
`irods_rs_image`                           | no       | ds-irods-rs-onbuild                  |         | The name of the unpublished RS image to be generated
`irods_server_control_plane_key`           | no       | TEMPORARY__32byte_ctrl_plane_key     |         | The server control plane key
`irods_server_port_range_end`              | no       | 20199                                |         | The last address in the range of auxillary TCP and UDP ports
`irods_server_port_range_start`            | no       | 20000                                |         | The first address in the range of auxillary TCP and UDP ports
`irods_service_account_name`               | no       | irods                                |         | The system account used to run the iRODS server processes
`irods_service_group_name`                 | no       | `irods_service_account_name`         |         | The system group used to run the iRODS server processes
`irods_storage_resources`                  | no       | []                                   |         | A list of storage resources hosted on the server being configured, _see below_
`irods_user_password_salt`                 | no       |                                      |         | The salt used when obfuscating user passwords stored in the catalog database
`irods_version`                            | no       | 4.2.8                                |         | The version of iRODS to work with
`irods_zone_key`                           | no       | TEMPORARY_zone_key                   |         | The zone key
`irods_zone_name`                          | no       | tempZone                             |         | The name of the zone
`mdrepo_clie_account`                      | no       | md-cli if mdrepo_collection exists   |         | The iRODS account used my the MD Repo CLI
`mdrepo_collection`                        | no       | null                                 |         | The base collection path for the MD Repo service
`mdrepo_landing_collection`                | no       | ''                                   |         | The collection path where user data are uploaded relative to `mdrepo_collection`
`mdrepo_manager`                           | no       | `irods_admin_username`               |         | The iRODS account of the user who is responsible for the MD Repo data
`mdrepo_svc_account`                       | no       | md-svc if mdrepo_collection exists   |         | The iRODS account used by the MD Repo service
`mdrepo_svc_password`                      | maybe    |                                      |         | The password used to authenticate `mdrepo_svc_account`, required if mdrepo_svc_account is not null
`pire_manager`                             | no       | null                                 |         | The username that owns the PIRE project collection, if `null`, the collection isn't created.
`pire_resource_hierarchy`                  | no       | `irods_resource_hierarchies[0]`      |         | The resource used by the PIRE project
`report_email_addr`                        | no       | root@localhost                       |         | The address where reports are to be emailed.
`restart_irods`                            | no       | false                                |         | iRODS can be restarted on the servers having config file changes, _see below_
`sernec_owners`                            | no       | []                                   |         | A list of users who get ownership of sernec collections
`sernec_readers`                           | no       | []                                   |         | A list of users who get read access to sernec collections
`sernec_writers`                           | no       | []                                   |         | A list of users who get write access to sernec collections
`sparcd_admin`                             | no       | null                                 |         | The user name of the Sparc'd administrator. If this isn't set, no sparcd rules will fire.
`sparcd_base_collection`                   | no       | _see description_                    |         | The base iRODS collection used by Sparc'd. If `sparcd_admin` is `null`, the default is `null`, otherwise it is `/{{ irods_zone_name }}/home/{{ sparcd_admin }}/Sparcd/Collections`.
`sparcd_report_email_addr`                 | no       | _see description_                    |         | The email address where SPARC'd notifications are sent. If `sparcd_admin` is `null`, the default is `null`, otherwise it is `report_email_addr`.
`sysctl_kernel`                            | no       | []                                   |         | A list of sysctl kernel parameters to set on the iRODS catalog service provider, _see_below_
`terraref_base_collection`                 | no       |                                      |         | The base collection for the TerraREF project. If it isn't present no TerraREF rules will fire.
`terraref_manager`                         | no       | `irods_clerver_user`                 |         | The iRODS user who is responsible for TerraREF data.
`terraref_resource_hierarchy`              | no       | `irods_resource_hierarchies[0]`      |         | The resource used by the TerraREF project.
`webdav_access_limit`                      | no       |                                      |         | If defined, the upper limit on the number of simultaneous requests that will be served by webdav
`webdav_allowed_src`                       | no       | `[ "0.0.0.0/0" ]`                    |         | A list of network/masks for the clients allowed direct access to the WebDAV servers
`webdav_auth_name`                         | no       | CyVerse                              |         | Authorization realm to use for the Data Store
`webdav_cache_dir`                         | no       | /var/cache/varnish                   |         | The directory varnish-cache will use for the WebDAV cache
`webdav_cache_max_file_size`               | no       | 10                                   |         | The maximum size in mebibytes of the largest WebDAV file varnish-cache will cache
`webdav_cache_max_ttl`                     | no       | 86400                                |         | The maximum cache TTL in seconds
`webdav_cache_size`                        | no       | 1000                                 |         | The maximum size in mebibytes the cache can be
`webdav_cache_ttl_fraction`                | no       | 0.1                                  |         | The fraction elapsed time since the last-modified time of a file for cache TTL (Time-to-live) configuration
`webdav_canonical_hostname`                | no       | `canonical_hostname`                 |         | The FQDN or IP address of the WebDAV service.
`webdav_davrods_access_limit`              | no       |                                      |         | If defined, the upper limit on the number of simultaneous requests that will be served by davrods
`webdav_max_request_workers`               | no       | 192                                  |         | The upper limit on the number of simultaneous requests that will be served. This typically have the value of `webdav_server_limit` multiplied by `webdav_threads_per_child`
`webdav_purgeman_irods_user`               | no       | `irods_admin_username`               |         | The irods user who converts data object uuid to path
`webdav_purgeman_irods_password`           | yes      |                                      |         | The password of the purgeman irods user
`webdav_server_limit`                      | no       | 48                                   |         | the number of cpu cores to be used
`webdav_threads_per_child`                 | no       | 4                                    |         | the number of threads per core to be created
`webdav_tls_cert`                          | no       |                                      |         | The TLS certificate file contents
`webdav_tls_cert_file`                     | no       | /etc/ssl/certs/dummy.crt             |         | The TLS certificate file used for encrypted communication
`webdav_tls_chain`                         | no       |                                      |         | The TLS certificate chain file contents
`weddav_tls_chain_file`                    | no       | /etc/ssl/certs/dummy-chain.crt       |         | The TLS certificate chain file used for encrypted communication
`webdav_tls_key`                           | no       |                                      |         | The TLS key
`webdav_tls_key_file`                      | no       | /etc/ssl/certs/dummy.key             |         | The TLS key file used for encrypted communication
`webdav_varnish_service_port`              | no       | 6081                                 |         | The service port number for varnish-cache
`sftp_port`                                | no       | 2022                                 |         | The SFTP service port number
`sftp_proxy_allowed`                       | no       | `[]`                                 |         | A list of network/masks for the proxy servers allowed access to the SFTP servers
`sftpgo_admin_ui_port`                     | no       | 18023                                |         | The SFTPGo admin UI service port number
`sftpgo_vault_dir`                         | no       | /sftpgo_vault                        |         | The directory SFTPGo will use for saving state
`sftpgo_admin_username`                    | no       | admin                                |         | The SFTPGo admin account name
`sftpgo_admin_password`                    | yes      |                                      |         | The password of the SFTPGo admin user
`sftpgo_irods_proxy_username`              | no       | sftp                                 |         | The irods user who provides proxy access to SFTPGo
`sftpgo_irods_proxy_password`              | yes      |                                      |         | The password of the SFTPGo irods proxy user

`irods_federatation` entry fields

All of them are required.

Field                    | Comments
------------------------ | --------
`catalog_provider_hosts` | A list of the catalog service providers in the federate, each indicated by its FQDN or IP address
`negotiation_key`        | The 32-byte encryption key of the federate
`zone_key`               | The shared authentication secret of the federate
`zone_name`              | The name of the federated zone

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
