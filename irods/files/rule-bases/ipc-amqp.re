# The Data Store logic for sending AMQP messages
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona. 
# For license information, see https://cyverse.org/license.


# sends a message to a given AMQP topic exchange
#
# Parameters:
#  *Key  (string) the topic of the message
#  *Msg  (string) the message to send
#
# Remote Execution:
#  It executes the amqptopicsend.py command script on the rule engine host
#
ipc_amqpSend(*Key, *Msg) {
  *exchangeArg = execCmdArg(ipc_AMQP_EXCHANGE);
  *keyArg = execCmdArg(*Key);
  *msgArg = execCmdArg(*Msg);
  *argStr = '*exchangeArg *keyArg *msgArg';

  *status = errormsg(
    msiExecCmd('amqptopicsend.py', *argStr, ipc_RE_HOST, 'null', 'null', *out), *errMsg );

  if (*status < 0) {
    writeLine("serverLog", "Failed to send AMQP message: *errMsg");
  }

  *status;
}
