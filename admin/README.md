# CyVerse DS System Administration playbooks

This is a collection of playbooks for maintaining DS servers.

## Tags

* `no_testing` for tasks that shouldn't be run within the containerized testing environment
* `non_idempotent` for tasks that aren't idempotent

## Variables

Variable           | Required | Default | Choices | Comments
------------------ | -------- |--------| ------- | --------
`domain_name`      | yes      |        |         | The public FQDN for the environment being configured. This is used for configuring services that require a public domain to work, like mail.
`maintainer_keys`  | no       | []     |         | A list of public ssh keys allowed to connect as the `ansible_user` on all of the managed hosts
`nic_mtu`          | no       | 1500   |         | The MTU to set on the primary NIC
`nic_txqueuelen`   | no       | 1000   |         | The transmission queue length to set on the primary NIC
`package_manager`  | no       | auto   |         | The packager manager to use
`rebootable`       | no       | `true` |         | Whether or not the server being configured is rebootable
`sysctl_net`       | no       | []     |         | a list of sysctl network parameters to set for the server being configured, __see_below__
`update_e2fsprogs` | no       | `true` |         | Whether or not the e2fsprogs system package can be upgraded on the server being configured

`sysctl_net` entry fields

Both of them are required.

Field    | Comments
-------- | --------
`name`   | The parameter name to modify
`value`  | The new value to set
