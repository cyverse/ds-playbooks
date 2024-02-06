# CyVerse DS AMQP Playbooks

This is a collection of playbooks for deploying a AMQP broker for the CyVerse Data Store.

## Variables

None of these variables are required.

Variable               | Default | Choices | Comments
---------------------- | ------- | ------- | --------
`amqp_admin_username`  | guest   |         | The AMQP broker admin user
`amqp_admin_password`  | guest   |         | The password for `amqp_admin_username`
`amqp_broker_port`     | 5672    |         | The port used by the broker
`amqp_management_port` | 15672   |         | The port used by the management interface

## Testing

Testing of the playbook logic is not done using the testing env (`../testing`). Instead molecule is
used. The molecule testing logic is in the rabbitmq role in the folder `roles/rabbitmq/molecule`.
