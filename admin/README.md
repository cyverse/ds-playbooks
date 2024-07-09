# CyVerse DS System Administration playbooks

This is a collection of playbooks for maintaining DS servers.

## Tags

* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

The following variables can be set in the inventory.

Variable               | Required | Default | Choices | Comments
---------------------- |----------|---------|---------|---------
`domain_name`          | yes      |         |         | The public FQDN for the environment being configured. This is used for configuring services that require a public domain to work, like mail.
`maintainer_keys`      | no       | []      |         | A list of public ssh keys allowed or disallowed to connect as the `ansible_user` on all of the managed hosts, __see below__
`nic_mtu`              | no       | 1500    |         | The MTU to set on the primary NIC
`nic_txqueuelen`       | no       | 1000    |         | The transmission queue length to set on the primary NIC
`package_manager`      | no       | auto    |         | The packager manager to use
`proxied_ssh`          | no       | false   |         | Whether or not the connection ansible uses to get to the managed node goes through a bastion host
`reboot_on_pkg_change` | no       | false   |         | Whether or not to automatically reboot the host if a system package was upgrade
`rebootable`           | no       | true    |         | Whether or not the server being configured is rebootable
`sysctl_net`           | no       | []      |         | a list of sysctl network parameters to set for the server being configured, __see below__

A `maintainer_key` is either a string or a maintainer_key mapping with the following fields.

Field   | Required | Default | Choices        | Comments
--------|----------|---------|----------------|---------
`key`   | yes      |         |                | The public ssh key
`state` | no       | present | absent,present | 'present' indicates that this can be used to authorize a connection and 'absent' indicates the opposite

If `maintainer_key` is a string, its value is assumed to be a public ssh key that can be used to authorized a connection.

`sysctl_net` entry fields

Both of them are required.

Field   | Comments
--------|---------
`name`  | The parameter name to modify
`value` | The new value to set
