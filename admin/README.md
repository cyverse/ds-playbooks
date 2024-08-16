# CyVerse DS System Administration playbooks

This is a collection of playbooks for maintaining DS servers.

## Tags

* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable                          | Required | Default | Choices | Comments
----------------------------------|----------|---------|---------|---------
`ansible_support_package_manager` |          | no      | auto    | The package manager to use
`connectivity_maintainer_keys`    | no       | []      |         | A list of public ssh keys allowed or disallowed to connect as the `ansible_user` on all of the managed hosts, __see below__
`connectivity_proxied_ssh`        | no       | false   |         | Whether or not the connection ansible uses to get to the managed node goes through a bastion host
`mail_domain_name`                | yes      |         |         | The public FQDN for the environment being configured. This is used for configuring services that require a public domain to work, like mail.
`network_mtu`                     | no       | 1500    |         | The MTU to set on the primary NIC
`network_sysctl`                  | no       | []      |         | a list of sysctl network parameters to set for the server being configured, __see below__
`network_txqueuelen`              | no       | 1000    |         | The transmission queue length to set on the primary NIC
`reboot_rebootable`               | no       | true    |         | Whether or not the server being configured is rebootable

An element of `connectivity_maintainer_keys` is either a string or a mapping with the following fields.

Field   | Required | Default | Choices        | Comments
--------|----------|---------|----------------|---------
`key`   | yes      |         |                | The public ssh key
`state` | no       | present | absent,present | 'present' indicates that this can be used to authorize a connection and 'absent' indicates the opposite

If it is a string, its value is assumed to be a public ssh key that can be used to authorized a connection.

`network_sysctl` entry fields

Both of them are required.

Field   | Comments
--------|---------
`name`  | The parameter name to modify
`value` | The new value to set
