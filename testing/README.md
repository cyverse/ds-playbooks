# testing

This folder contains the test harness for the DS playbooks. It consists of a
simplified Data Store and a playbook runner.  At the moment, it doesn't do
check to see if the tests passed. That must be done by inspection.

## The Environment

The environment consists of five containers. The `amqp` container hosts the
RabbitMQ broker that in turn hosts the `irods` exchange, where the Data Store
publishes messages to. The `dbms` container hosts the PostgreSQL server that in
turn hosts the ICAT DB. The `ies` container hosts the IES. The `rs_centos6`
container hosts the resource server running on CentOS 6. Finally, the
`rs_centos7` container hosts the resource server running on CentOS 7.

## Building the Harness

There are two convenience scripts for building the docker images for the
environment. `build` builds all the required images, and `clean` deletes them.

## Testing

`run` is a convenience script for running a playbook against the test
environment. This script will bring up the environment, run the playbooks, and
then tear down the environment.  

__TODO add support for test validation yet.__
