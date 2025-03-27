# CyVerse DS iRODS Playbooks

This is a collection of playbooks for deploying iRODS for the CyVerse Data Store.

## Tags

* `firewall` for tasks related to firewall configuration
* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable                                   | Required | Default                              | Choices | Comments
------------------------------------------ | -------- | ------------------------------------ | ------- | --------
`irods_amqp_exchange`                      | no       | irods                                |         | The AMQP exchange used to publish events
`irods_amqp_host`                          | no       | `groups['amqp'][0]`                  |         | the FQDN or IP address of the server hosting the AMQP service
`irods_amqp_mgmt_port`                     | no       | 15672                                |         | The TCP port used for management of the AMQP vhost
`irods_amqp_password`                      | no       | guest                                |         | The password iRODS uses to connect to the AMQP vhost
`irods_amqp_port`                          | no       | 5672                                 |         | The TCP port the RabbitMQ broker listens on
`irods_amqp_user`                          | no       | guest                                |         | The user iRODS uses to connect to the AMQP vhost
`irods_amqp_vhost`                         | no       | /                                    |         | The AMQP vhost iRODS connects to
`irods_allowed_clients`                    | no       | 0.0.0.0/0                            |         | The network/mask for the clients allowed to access iRODS.
`irods_canonical_hostname`                 | no       | `groups['irods_catalog'][0]`         |         | The external FQDN used to access the data store services
`irods_canonical_zone_port`                | no       | 1247                                 |         | The port on the `irods_canonical_hostname` host listening for connections to iRODS
`sftp_admin_password`                      | yes      |                                      |         | The password of the SFTPGo admin user
`sftp_admin_ui_port`                       | no       | 18023                                |         | The SFTPGo admin UI service port number
`sftp_admin_username`                      | no       | admin                                |         | The SFTPGo admin account name
`sftp_irods_auth_scheme`                   | no       | native                               |         | The auth scheme of irods. 'pam' and 'pam_for_users' are also available.
`sftp_irods_proxy_password`                | yes      |                                      |         | The password of the SFTPGo irods proxy user
`sftp_irods_proxy_username`                | no       | sftp                                 |         | The irods user who provides proxy access to SFTPGo
`sftp_irods_ssl_algorithm`                 | no       |                                      |         | The SSL encryption algorithm (required by PAM auth scheme)
`sftp_irods_ssl_ca_cert_path`              | no       |                                      |         | The SSL CA certificate file path (required by PAM auth scheme)
`sftp_irods_ssl_hash_rounds`               | no       |                                      |         | The SSL encryption hash rounds (required by PAM auth scheme)
`sftp_irods_ssl_key_size`                  | no       |                                      |         | The SSL encryption key size (required by PAM auth scheme)
`sftp_irods_ssl_salt_size`                 | no       |                                      |         | The SSL encryption salt size (required by PAM auth scheme)
`sftp_port`                                | no       | 2022                                 |         | The SFTP service port number
`sftp_proxy_allowed`                       | no       | `[]`                                 |         | A list of network/masks for the proxy servers allowed access to the SFTP servers
`sftp_user_host_allowed`                   | no       | `[]`                                 |         | A list of ip addresses of the user hosts allowed (whitelisted) for access to the SFTP servers
`sftp_user_host_rejected`                  | no       | `[]`                                 |         | A list of ip addresses of the user hosts rejected (blacklisted) for access to the SFTP servers
`sftp_vault_dir`                           | no       | /sftpgo_vault                        |         | The directory SFTPGo will use for saving state
`webdav_access_limit`                      | no       |                                      |         | If defined, the upper limit on the number of simultaneous requests that will be served by webdav
`webdav_allowed_src`                       | no       | `[ "0.0.0.0/0" ]`                    |         | A list of network/masks for the clients allowed direct access to the WebDAV servers
`webdav_auth_name`                         | no       | CyVerse                              |         | Authorization realm to use for the Data Store
`webdav_cache_dir`                         | no       | /var/cache/varnish                   |         | The directory varnish-cache will use for the WebDAV cache
`webdav_cache_max_file_size`               | no       | 10                                   |         | The maximum size in mebibytes of the largest WebDAV file varnish-cache will cache
`webdav_cache_max_ttl`                     | no       | 86400                                |         | The maximum cache TTL in seconds
`webdav_cache_size`                        | no       | 1000                                 |         | The maximum size in mebibytes the cache can be
`webdav_cache_ttl_fraction`                | no       | 0.1                                  |         | The fraction elapsed time since the last-modified time of a file for cache TTL (Time-to-live) configuration
`webdav_canonical_hostname`                | no       | `irods_canonical_hostname`           |         | The FQDN or IP address of the WebDAV service.
`webdav_davrods_access_limit`              | no       |                                      |         | If defined, the upper limit on the number of simultaneous requests that will be served by davrods
`webdav_max_request_workers`               | no       | 192                                  |         | The upper limit on the number of simultaneous requests that will be served. This typically have the value of `webdav_server_limit` multiplied by `webdav_threads_per_child`
`webdav_purgeman_irods_user`               | no       | `irods_admin_username`               |         | The irods user who converts data object uuid to path
`webdav_purgeman_irods_password`           | yes      |                                      |         | The password of the purgeman irods user
`webdav_server_limit`                      | no       | 48                                   |         | the number of cpu cores to be used
`webdav_threads_per_child`                 | no       | 4                                    |         | the number of threads per core to be created
`webdav_tls_cert`                          | no       |                                      |         | The TLS certificate file contents
`webdav_tls_cert_file`                     | no       | /etc/ssl/certs/dummy.crt             |         | The TLS certificate file used for encrypted communication
`webdav_tls_chain`                         | no       |                                      |         | The TLS certificate chain file contents
`webdav_tls_chain_file`                    | no       | /etc/ssl/certs/dummy-chain.crt       |         | The TLS certificate chain file used for encrypted communication
`webdav_tls_key`                           | no       |                                      |         | The TLS key
`webdav_tls_key_file`                      | no       | /etc/ssl/certs/dummy.key             |         | The TLS key file used for encrypted communication
`webdav_varnish_service_port`              | no       | 6081                                 |         | The service port number for varnish-cache
