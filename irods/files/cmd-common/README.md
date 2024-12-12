# General Purpose iRODS Command Scripts

This folder contains the command scripts executable through the `msiExecCmd` microservice. Only the general purpose ones are here. The ones used by optional components of the Data Store are stored elsewhere.

* [amqp-topic-send](amqp-topic-send) publishes audit messages to a RabbitMQ broker.
* [correct-size](correct-size) fixes data object replica sizes as a workaround for <https://github.com/irods/irods/issues/5160>. _Once this issue is fixed, this should be removed._
* [delete-scheduled-rule](delete-scheduled-rule) removes a rule execution from the rule queue.
* [generate-uuid](generate-uuid) generates a time-based UUID.
* [imeta-exec](imeta-exec) calls imeta.
* [send-mail](send-mail) sends an email message.
