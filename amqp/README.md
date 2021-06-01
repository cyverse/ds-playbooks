# CyVerse DS AMQP Playbooks

This is a collection of playbooks for deploying a AMQP broker for the CyVerse Data Store.

## Tags

* `no_testing` will skip tasks that shouldn't be run within the containerized testing environment

## Variables

None of these variables are required.

Variable               | Default | Choices | Comments
---------------------- | ------- | ------- | --------
`amqp_admin_username`  | guest   |         | The AMQP broker admin user
`amqp_admin_password`  | guest   |         | The password for `amqp_admin_username`
`amqo_broker_port`     | 5672    |         | The port used by the broker
`amqp_management_port` | 15672   |         | The port used by the management interface
