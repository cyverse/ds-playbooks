# CyVerse DS System Administration playbooks

This is a collection of playbooks for maintaining DS servers.


## Variables

None of these variables are required.

Variable           | Default | Choices | Comments
------------------ | --------| ------- | --------
`nic_mtu`          | 1500    |         | The MTU to set on the primary NIC
`nic_txqueuelen`   | 1000    |         | The transmission queue length to set on the primary NIC
`rebootable`       | true    |         | Whether or not the server being configured is rebootable
`sysctl_net`       | []      |         | a list of sysctl network parameters to set for the server being configured, __see_below__
`update_e2fsprogs` | true    |         | Whether or not the e2fsprogs system package can be upgraded on the server being configured

`sysctl_net` entry fields

Both of them are required.

Field    | Comments
-------- | --------
`name`   | The parameter name to modify
`value`  | The new value to set
