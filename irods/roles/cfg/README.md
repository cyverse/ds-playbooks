ansible-cyverse-irods-cfg
=========================

This is a role for configuring CyVerse iRODS 4 grids.


Requirements
------------

It is assumed that iRODS 4.1.10 is already installed on the server.
The server needs to dnspython module installed.

Role Variables
--------------

Variable                                  | Required | Default                          | Choices                         | Comments
----------------------------------------- | -------- | -------------------------------- | ------------------------------- | --------
`cfg_aegis_resource_name`               | no       | aegisIngestRes                   |                                 | the name of the aegis resource where newly uploaded files are written
`cfg_aegis_repl_resource_name`          | no       | aegisReplRes                     |                                 | the name of the aegis resource where replicas are written
`cfg_icat_host`                         | no       | localhost                        |                                 | the FQDN of the IES
`cfg_client_server_negotiation`         | no       | request_server_negotiation       | request_server_negotiation, off | indicates whether or not to use advanced negotiation
`cfg_default_number_of_transfer_threads | no       | 4                                |                                 | the default maximum number of transfer streams for parallel transfer
`cfg_default_resource_directory`        | no       |                                  |                                 | the absolute path to the vault on the resource server being configured (N/A when configuring IES and it doesn't host a resource)
\cfg_default_repl_resource_name`        | no       | `cfg_default_resource_name`    |                                 | the default resource for replication
`cfg_default_resource_name`             | no       | demoResc                         |                                 | the name of the default resource
`cfg_negotiation_key`                   | no       | TEMPORARY_32byte_negotiation_key |                                 | the negotiation key
`cfg_server_control_plane_key`          | no       | TEMPORARY__32byte_ctrl_plane_key |                                 | the server control plane key
`cfg_parallel_transfer_buffer_size      | no       | 4                                |                                 | the transfer buffer size in MiB for each stream during parallel transfer
`cfg_server_port_range_start`           | no       | 20000                            |                                 | the first address in the range of auxillary TCP and UDP ports
`cfg_server_port_range_end`             | no       | 20199                            |                                 | the last address in the range of auxillary TCP and UDP ports
`cfg_zone_key`                          | no       | TEMPORARY_zone_key               |                                 | the zone key
`cfg_zone_user`                         | no       | rods                             |                                 | the rodsadmin user to be used by the server being configured
`cfg_db`                                | no       |                                  |                                 | the DBMS connection information, see below (N/A for non-IES resource servers)
`cfg_federation`                        | no       | []                               |                                 | a list of other iRODS zones to federate with, see below
`cfg_amqp_uri`                          | no       |                                  |                                 | the AMQP URI used to connect to the broker (N/A for non-IES resource servers)
`cfg_amqp_ephemeral`                    | no       | true                             |                                 | whether or not the `irods` AMQP exchange will persist when iRODS disconnects from the AMQP broker
`cfg_single_threaded_resources`         | no       | []                               |                                 | a list of resources that only support single threaded transfers
`irods_bisque_irods_host`                 | no       | `cfg_icat_host`                |                                 | The iRODS host to report to BisQue.
`cfg_sernec_owners`                     | no       | []                               |                                 | a list of users who get ownership of sernec collections
`cfg_sernec_writers`                    | no       | []                               |                                 | a list of users who get write access to sernec collections
`cfg_sernec_readers`                    | no       | []                               |                                 | a list of users who get read access to sernec collections


`cfg_db` fields

Variable   | Required | Default | Choices | Comments
-----------| -------- | ------- | ------- | --------
`host`     | yes      |         |         | the FQDN of the DBMS hosting the ICAT
`port`     | yes      |         |         | the port the DBMS listens on
`username` | yes      |         |         | the DBMS user iRODS uses
`password` | yes      |         |         | the password for the DBMS user iRODS uses


`irods_federate fields

Variable          | Required | Default | Choices | Comments
------------------| -------- | ------- | ------- | --------
`icat_host`       | yes      |         |         | the hostname of the IES in the federate
`zone_name`       | yes      |         |         | the name of the federated zone
`zone_key`        | yes      |         |         | the shared authentication secret of the federate
`negotiation_key` | yes      |         |         | the 32-byte encryption key of the federate


Dependencies
------------

This role depends on [CyVerse-Ansible.irods-cfg](https://galaxy.ansible.com/CyVerse-Ansible/irods-cfg/) for the generation and deposition of the server's irods_environment.json and server_config.json.


Example Playbook
----------------

Including an example of how to use your role (for instance, with variables passed in as parameters) is always nice for users too:

```
- hosts: irods-servers
  become_user: irods
  gather_facts: true
  roles:
    - role: cyverse-irods-cfg
      cfg_amqp_uri: amqp://guest:guest@localhost:5672/%2F
      cfg_db:
        host: localhost
        port: 5432
        username: irodsuser
        password: password
```           

License
-------

See license.md


Author Information
------------------

Tony Edgin
