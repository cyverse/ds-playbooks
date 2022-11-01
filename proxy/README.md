# CyVerse DS Proxy Playbooks

This is a collection of  playbooks for maintaining the Proxy for the Data Store.

## Playbooks

* `main.yml` completely deploys the proxies
* `start.yml` starts HAProxy
* `stop.yml` stops HAProxy
* `terminate_connections.yml` terminates all client connections to HAProxy and blocks new
     connections
* `unblock_access.yml` allows client connections to be made

## Tags

* `no_testing` for tasks that shouldn't run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable                       | Default                     | Comments
------------------------------ | --------------------------- | --------
`proxy_rsyslog_conf`           | /etc/rsyslog.d/haproxy.conf | the path to the rsyslog configuration file for HAProxy
`proxy_stats_auth`             | null                        | an object providing the authentication credentials for the HAProxy stats web interface _see below_
`proxy_stats_tls_crt`          | null                        | the absolute path to the TLS certificate chain used for securing the HAProxy stats web interface
`proxy_stats_tls_crt_content`  | null                        | the content of the TLS certificate chain file
`proxy_irods_direct_max_conn`  | 200                         | the maximum number of connections to iRODS
`proxy_irods_reconn_ports`     | 20000-20399                 | the range of TCP range of ports that need to be forwarded to iRODS for reconnections
`proxy_irods_vip_client_hosts` | []                          | a list of host names, ip addresses, or CIDR blocks of clients allowed unlimited concurrent iRODS connections.
`proxy_sftp_port`              | 22                          | the TCP port hosting the SFTP service whose communication will be forwarded to SFTPGo

`proxy_stats_auth` object fields

Field      | Required | Default | Comments
---------- | -------- | ------- | --------
`username` | no       | ds      | the account authorized to access the stats web interface
`password` | yes      |         | the password used to authenticate the account
`realm`    | no       |         | the realm of the authentication system
