# CyVerse DS RabbitMQ Playbooks

This is a collection of playbooks for deploying a RabbitMQ broker for the CyVerse Data Store.


## Variables

None of these variables are required.

Variable                  | Default | Choices | Comments
------------------------- | ------- | ------- | --------
`rabbitmq_admin_password` | guest   |         | The password for `rabbitmq_admin_user`
`rabbitmq_admin_user`     | guest   |         | The RabbitMQ broker admin user for the / vhost
`rabbitmq_deploy_env`     | testing |         | The name of this CyVerse deployment environment
`rabbitmq_ds_exchange`    | irods   |         | The name of exchanged used by the Data Store
