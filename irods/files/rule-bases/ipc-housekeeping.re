# VERSION 3
#
# ipc-housekeeping.re
# This is a library of rules for periodic tasks like updating quota usage data.


_ipc_schedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
  writeLine('serverLog', 'DS: scheduling *Desc');
  eval(``delay('<PLUSET>0s</PLUSET><EF>*Freq</EF>') {`` ++ *RuleName ++ ``}``);
}


_ipc_reschedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
  *scheduled = false;

  foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY WHERE RULE_EXEC_NAME = '*RuleName|') {
    if (*scheduled || *row.RULE_EXEC_FREQUENCY != *Freq) {
      writeLine('serverLog', 'DS: unscheduling *Desc');

      *idArg = execCmdArg(*row.RULE_EXEC_ID);

      *status = errorcode(
        msiExecCmd('delete-scheduled-rule', *idArg, 'null', 'null', 'null', *out));

      if (*status < 0) {
        msiGetStderrInExecCmdOut(*out, *resp);
        failmsg(*status, *resp);
      }
    } else {
      *scheduled = true;
    }
  }

  if (*scheduled) {
    writeLine('stdout', '*Desc already scheduled');
  } else {
    _ipc_schedulePeriodicPolicy(*RuleName, *Freq, *Desc);
    writeLine('stdout', 'scheduled *Desc');
  }
}


#
# QUOTAS
#

_ipc_updateQuotaUsage {
  writeLine('serverLog', 'DS: updating quota usage');

  if (0 == errormsg(msiQuota, *msg)) {
    writeLine('serverLog', 'DS: quota usage updated');
  } else {
    writeLine('serverLog', "DS: quota usage update failed: *msg");
  }
}


# This rule shedules the hourly calculation of quota usage data. If it
# reschedules the calculation, it writes 'scheduled quota usage updates' to
# standard output. If it doesn't error out, but doesn't reschedule the
# calculation, it writes 'quota usage updates already scheduled'.
#
ipc_rescheduleQuotaUsageUpdate {
  _ipc_reschedulePeriodicPolicy(
    ``_ipc_updateQuotaUsage``, '1h REPEAT FOR EVER', 'quota usage updates');
}


#
# STORAGE FREE SPACE
#

# NOTE: This runs on the resource server hosting the resource whose free space
#       is in question.
_ipc_determineStorageFreeSpace(*Host, *RescName) {
  remote(*Host, '') {
    if (0 == errormsg(msi_update_unixfilesystem_resource_free_space(*RescName), *msg)) {
      writeLine('serverLog', "DS: determined free space on *RescName");
    } else {
      writeLine('serverLog', "DS: failed to determine free space on *RescName: *msg");
    }
  }
}


_ipc_determineAllStorageFreeSpace {
  writeLine('serverLog', 'DS: determining free space on resource servers');

  foreach(*record in SELECT RESC_NAME, RESC_LOC
                     WHERE RESC_TYPE_NAME = 'unixfilesystem' AND RESC_STATUS = 'up') {
    _ipc_determineStorageFreeSpace(*record.RESC_LOC, *record.RESC_NAME);
  }

  writeLine('serverLog', 'DS: determined free space on resource servers');
}


# This rule schedules the daily determination of the available disk space for
# all Unix file system resources. If it reschedules the determination, it writes
# 'scheduled storage determination' to standard output. If it doesn't error
# out, but it doesn't reschedule the determination, it writes 'storage 
# determination already scheduled'.
#
ipc_rescheduleStorageFreeSpaceDetermination {
  _ipc_reschedulePeriodicPolicy(
    ``_ipc_determineAllStorageFreeSpace``, '1d REPEAT FOR EVER', 'storage determination');
}


#
# TRASH REMOVAL
#

_ipc_rmTrash {
  writeLine('serverLog', 'DS: starting trash removal');

  msiGetFormattedSystemTime(*date, 'human', '%d-%02d-%02d');
  *logFileArg = execCmdArg("/var/lib/irods/iRODS/server/log/trash-removal.log-*date");

  if (0 == errorcode(msiExecCmd('rm-trash', "--log *logFileArg", 'null', 'null', 'null', *out))) {
    *subject = 'DS: ' ++ ipc_ZONE ++ ' trash removal of succeeded';
    *body = 'SSIA';
  } else {
    *subject = 'DS: ' ++ ipc_ZONE ++ ' trash removal failed';
    msiGetStdErrInExecCmdOut(*out, *body);
  }

  if (0 != errorcode(msiSendMail(ipc_REPORT_EMAIL_ADDR, *subject, *body))) {
    writeLine('serverLog', 'DS: failed to mail trash removal report');
  }

  writeLine('serverLog', 'DS: completed trash removal');
}


# This rule shedules the weekly trash removal. If it reschedules the removal,
# it writes 'scheduled trash removal' to standard output. If it doesn't error
# out, but it doesn't reschedule the removal, it writes 'trash removal already
# scheduled'.
#
ipc_rescheduleTrashRemoval {
  _ipc_reschedulePeriodicPolicy(``_ipc_rmTrash``, '7d REPEAT FOR EVER', 'trash removal');
}
