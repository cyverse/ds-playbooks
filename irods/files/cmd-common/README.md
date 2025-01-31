# General Purpose iRODS Command Scripts

This folder contains the command scripts executable through the `msiExecCmd` microservice. Only the general purpose ones are here. The ones used by optional components of the Data Store are stored elsewhere.

* [amqp-topic-send](var/lib/irods/msiExecCmd_bin/amqp-topic-send) publishes audit messages to a RabbitMQ broker.
* [correct-size](var/lib/irods/msiExecCmd_bin/correct-size) fixes data object replica sizes as a workaround for <https://github.com/irods/irods/issues/5160>. _Once this issue is fixed, this should be removed._
* [delete-scheduled-rule](var/lib/irods/msiExecCmd_bin/delete-scheduled-rule) removes a rule execution from the rule queue.
* [generate-uuid](var/lib/irods/msiExecCmd_bin/generate-uuid) generates a time-based UUID.
* [imeta-exec](var/lib/irods/msiExecCmd_bin/imeta-exec) calls imeta.
* [send-mail](var/lib/irods/msiExecCmd_bin/send-mail) sends an email message.
