# VERSION 1
#
# ipc-housekeeping.re
# This is a library of rules for periodic tasks like updating quota usage data.

_ipc_QUOTA_UPDATE_FREQ = '1h REPEAT FOR EVER'


_ipc_updateQuotaUsage {
  writeLine('serverLog', 'DS: updating quota usage');

  if (0 == errormsg(msiQuota, *msg)) {
    writeLine('serverLog', 'DS: quota usage updated');
  } else {
    writeLine('serverLog', 'DS: quota usage update failed: *msg');
  }
}


_ipc_scheduleQuotaUsageUpdate {
  writeLine('serverLog', 'DS: scheduling quota usage updates');
  delay('<PLUSET>0s</PLUSET><EF>' ++ _ipc_QUOTA_UPDATE_FREQ ++ '</EF>') {_ipc_updateQuotaUsage}
}


# This rule shedules the hourly calculation of quota usage data
#
ipc_rescheduleQuotaUsageUpdate {
  *scheduled = false;

  foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY
                  WHERE RULE_EXEC_NAME = '_ipc_updateQuotaUsage|') {
    if (*scheduled || *row.RULE_EXEC_FREQUENCY != _ipc_QUOTA_UPDATE_FREQ) {
      *idArg = execCmdArg(*row.RULE_EXEC_ID);

      writeLine('serverLog', 'DS: unscheduling quota usage updates');
      *status = errorcode(msiExecCmd('delete-scheduled-rule', *idArg, "null", "null", "null", *out));

      if (*status < 0) {
        msiGetStderrInExecCmdOut(*out, *resp);
        failmsg(*status, *resp);
      }
    } else {
      *scheduled = true;
    }
  }

  if (*scheduled) {
    writeLine('stdout', 'quota usage updates already scheduled');
  } else {
    writeLine('stdout', 'scheduling quota usage updates');
    _ipc_scheduleQuotaUsageUpdate;
  }
}
