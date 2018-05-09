# VERSION 1
#
# ipc-housekeeping.re
# This is a library of rules for periodic tasks like updating quota usage data.


_ipc_updateQuotaUsage {
  writeLine('serverLog', 'DS: updating quota usage');

  if (0 == errormsg(msiQuota, *msg)) {
    writeLine('serverLog', 'DS: quota usage updated');
  } else {
    writeLine('serverLog', 'DS: quota usage update failed: *msg');
  }
}


# This rule shedules the hourly calculation of quota usage data
#
ipc_scheduleQuotaUsageUpdate {
  writeLine('serverLog', 'DS: Scheduling quota usage updates');

  delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>') {
    _ipc_updateQuotaUsage;
  }
}
