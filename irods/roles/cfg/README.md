ansible-cyverse-irods-cfg
=========================

This is a role for configuring CyVerse iRODS 4 grids.


Requirements
------------

It is assumed that iRODS 4.1.10 is already installed on the server.
The server needs to dnspython module installed.

Role Variables
--------------

None of these variables are required.

Variable                                 | Default                          | Choices                         | Comments
---------------------------------------- | -------------------------------- | ------------------------------- | --------
`cfg_aegis_repl_resource_name`           | aegisReplRes                     |                                 | the name of the aegis resource where replicas are written
`cfg_aegis_resource_name`                | aegisIngestRes                   |                                 | the name of the aegis resource where newly uploaded files are written
`cfg_amqp_ephemeral`                     | true                             |                                 | whether or not the `irods` AMQP exchange will persist when iRODS disconnects from the AMQP broker
`cfg_amqp_uri`                           |                                  |                                 | the AMQP URI used to connect to the broker (N/A for non-IES resource servers)
`cfg_bisque_irods_host`                  | `cfg_ies_host`                |                                 | The iRODS host to report to BisQue.
`cfg_client_server_negotiation`          | request_server_negotiation       | request_server_negotiation, off | indicates whether or not to use advanced negotiation
`cfg_db`                                 |                                  |                                 | the DBMS connection information, see below (N/A for non-IES resource servers)
`cfg_default_number_of_transfer_threads` | 4                                |                                 | the default maximum number of transfer streams for parallel transfer
`cfg_default_repl_resource_name`         | `cfg_default_resource_name`    |                                 | the default resource for replication
`cfg_default_resource_directory`         |                                  |                                 | the absolute path to the vault on the resource server being configured (N/A when configuring IES and it doesn't host a resource)
`cfg_default_resource_name`              | demoResc                         |                                 | the name of the default resource
`cfg_federation`                         | []                               |                                 | a list of other iRODS zones to federate with, see below
`cfg_ies_host`                           | `inventory_hostname`                        |                                 | the FQDN of the IES
`cfg_max_num_re_procs`                   | 4                                |                                 | the maximum number of rule engine processes to run
`cfg_negotiation_key`                    | TEMPORARY_32byte_negotiation_key |                                 | the negotiation key
`cfg_parallel_transfer_buffer_size`      | 4                                |                                 | the transfer buffer size in MiB for each stream during parallel transfer
`cfg_sernec_owners`                      | []                               |                                 | a list of users who get ownership of sernec collections
`cfg_sernec_readers`                     | []                               |                                 | a list of users who get read access to sernec collections
`cfg_sernec_writers`                     | []                               |                                 | a list of users who get write access to sernec collections
`cfg_server_control_plane_key`           | TEMPORARY__32byte_ctrl_plane_key |                                 | the server control plane key
`cfg_server_port_range_end`              | 20199                            |                                 | the last address in the range of auxillary TCP and UDP ports
`cfg_server_port_range_start`            | 20000                            |                                 | the first address in the range of auxillary TCP and UDP ports
`cfg_single_threaded_resources`          | []                               |                                 | a list of resources that only support single threaded transfers
`cfg_zone_key`                           | TEMPORARY_zone_key               |                                 | the zone key
`cfg_zone_user`                          | rods                             |                                 | the rodsadmin user to be used by the server being configured


`cfg_db` fields

All of them are required.

Variable   | Comments
-----------| --------
`host`     | the FQDN of the DBMS hosting the ICAT
`password` | the password for the DBMS user iRODS uses
`port`     | the port the DBMS listens on
`username` | the DBMS user iRODS uses


`irods_federate fields`

All of them are required.

Variable          | Comments
----------------- | --------
`icat_host`       | the hostname of the IES in the federate
`negotiation_key` | the 32-byte encryption key of the federate
`zone_key`        | the shared authentication secret of the federate
`zone_name`       | the name of the federated zone


Dependencies
------------

This role depends on [CyVerse-Ansible.irods-cfg](https://galaxy.ansible.com/CyVerse-Ansible/irods-cfg/) for the generation and deposition of the server's irods_environment.json and server_config.json.


Example Playbook
----------------

```
- hosts: irods-servers
  become_user: irods
  gather_facts: true
  roles:
    - role: cyverse-irods-cfg
      vars:
        cfg_amqp_uri: amqp://guest:guest@localhost:5672/%2F
        cfg_db:
          host: localhost
          port: 5432
          username: irodsuser
          password: password
```
