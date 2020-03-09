# VERSION 2
#
# ipc-housekeeping.re
# This is a library of rules for periodic tasks like updating quota usage data.

_ipc_QUOTA_UPDATE_FREQ = '1h REPEAT FOR EVER'
_ipc_TRASH_RM_FREQ = '7d REPEAT FOR EVER'


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


_ipc_updateQuotaUsage {
  writeLine('serverLog', 'DS: updating quota usage');

  if (0 == errormsg(msiQuota, *msg)) {
    writeLine('serverLog', 'DS: quota usage updated');
  } else {
    writeLine('serverLog', "DS: quota usage update failed: *msg");
  }
}


_ipc_scheduleQuotaUsageUpdate {
  writeLine('serverLog', 'DS: scheduling quota usage updates');
  delay('<PLUSET>0s</PLUSET><EF>' ++ _ipc_QUOTA_UPDATE_FREQ ++ '</EF>') {_ipc_updateQuotaUsage}
}


_ipc_scheduleRmTrash {
  writeLine('serverLog', 'DS: scheduling trash removal');
  delay('<PLUSET>0s</PLUSET><EF>' ++ _ipc_TRASH_RM_FREQ ++ '</EF>') {_ipc_rmTrash}
}


# This rule shedules the hourly calculation of quota usage data
#
ipc_rescheduleQuotaUsageUpdate {
  *scheduled = false;

  foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY
                  WHERE RULE_EXEC_NAME = '_ipc_updateQuotaUsage|') {
    if (*scheduled || *row.RULE_EXEC_FREQUENCY != _ipc_QUOTA_UPDATE_FREQ) {
      writeLine('serverLog', 'DS: unscheduling quota usage updates');

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
    writeLine('stdout', 'quota usage updates already scheduled');
  } else {
    _ipc_scheduleQuotaUsageUpdate;
    writeLine('stdout', 'scheduled quota usage updates');
  }
}


# This rule shedules the weekly trash removal
#
ipc_rescheduleTrashRemoval {
  *scheduled = false;

  foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY WHERE RULE_EXEC_NAME = '_ipc_rmTrash|') {
    if (*scheduled || *row.RULE_EXEC_FREQUENCY != _ipc_TRASH_RM_FREQ) {
      writeLine('serverLog', 'DS: unscheduling trash removal');

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
    writeLine('stdout', 'trash removal already scheduled');
  } else {
    _ipc_scheduleRmTrash;
    writeLine('stdout', 'scheduled trash removal');
  }
}
