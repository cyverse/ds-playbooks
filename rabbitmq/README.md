# CyVerse DS RabbitMQ Playbooks

This is a collection of playbooks for deploying a RabbitMQ broker for the CyVerse Data Store.

## Tags

* `no_testing` for tasks that shouldn't be run within the containerized testing environment

## Variables

None of these variables are required.

Variable                   | Default | Choices | Comments
-------------------------- | ------- | ------- | --------
`rabbitmq_admin_password`  | guest   |         | The password for `rabbitmq_admin_user`
`rabbitmq_admin_user`      | guest   |         | The RabbitMQ broker admin user
`rabbitmq_ds_exchange`     | irods   |         | The name of exchanged used by the Data Store
`rabbitmq_ds_vhost`        | /       |         | The vhost used by the Data Store
`rabbitmq_broker_port`     | 5672    |         | The port used by the broker
`rabbitmq_management_port` | 15672   |         | The port used by the management interface
