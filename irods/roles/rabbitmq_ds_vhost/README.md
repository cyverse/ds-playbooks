# rabbitmq_ds_vhost

This role configures a RabbitMQ virtual host for the Data Store. It also creates the topic exchange
where iRODS will publish its event messages.

## Requirements

None

## Role Variables

Variable                           | Required | Default  | Choices | Comments
---------------------------------- | -------- | -------- | ------- | --------
`rabbitmq_ds_vhost_name`           | no       | /ds      |         | the name of the virtual host
`rabbitmq_ds_vhost_admin_username` | no       | ipc      |         | the RabbitMQ admin account
`rabbitmq_ds_vhost_admin_password` | yes      |          |         | the password used to authenticate the RabbitMQ admin
`rabbitmq_ds_vhost_exchange`       | no       | irods    |         | the exchange where iRODS will publish messages
`rabbitmq_ds_vhost_mgmt_port`      | no       | 15672    |         | the TCP port RabbitMQ uses for management

## Tags

* `no_testing` will skip tasks that shouldn't be run within the containerized testing environment

## Dependencies

None

## Example Playbook

```yaml
- hosts: amqp
  roles:
    - role: rabbitmq_ds_vhost
      vars:
        rabbitmq_ds_vhost_name: /prod/ds
        rabbitmq_ds_vhost_admin_password: crack-me
```
