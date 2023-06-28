# VERSION 3
#
# ipc-housekeeping.re
# This is a library of rules for periodic tasks like updating quota usage data.


_ipc_schedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
  writeLine('serverLog', 'DS: scheduling *Desc');
  # XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See
  #       https://github.com/irods/irods/issues/5413.
  #eval(``delay('<PLUSET>0s</PLUSET><EF>*Freq</EF>') {`` ++ *RuleName ++ ``}`` );
  eval(
    ``delay(
        '<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
         <PLUSET>0s</PLUSET>
         <EF>*Freq</EF>'
      ) {`` ++ *RuleName ++ ``}`` );
}


_ipc_reschedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
  *scheduled = false;

  foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY WHERE RULE_EXEC_NAME = '*RuleName') {
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
  writeLine('serverLog', "DS: remotely determining free space on *Host for *RescName");

  remote(*Host, '') {
    writeLine('serverLog', "DS: locally determining free space for *RescName");

    if (0 == errormsg(msi_update_unixfilesystem_resource_free_space(*RescName), *msg)) {
      writeLine('serverLog', "DS: determined free space for *RescName");
    } else {
      writeLine('serverLog', "DS: failed to determine free space for *RescName: *msg");
    }
  }
}


_ipc_determineAllStorageFreeSpace {
  writeLine('serverLog', 'DS: determining free space on resource servers');

  foreach(*record in SELECT RESC_LOC, RESC_NAME
                     WHERE RESC_TYPE_NAME = 'unixfilesystem' AND RESC_STATUS = 'up') {
    *host = *record.RESC_LOC;
    *resc = *record.RESC_NAME;

    if (0 > errormsg(_ipc_determineStorageFreeSpace(*host, *resc), *msg)) {
      writeLine('serverLog', "DS: failed to determine free space on *host for *resc: *msg");
    }
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
  *zone = ipc_ZONE;
  *verdict = true;
  msiGetSystemTime(*timestamp, "");

  # 2,592,000 is the number of seconds in 30 days. We subtract this value from the current timestamp
  # to calculate the threshold time for items in the trash that are older than 30 days.
  *int_month_timestamp = int(*timestamp) - 2592000;

  # iRODS appends a leading 0 to epoch timestamps, but the int conversion removes it.
  # To enable string comparison done below, we add a leading 0 to the month_timestamp string.
  *month_timestamp = '0'++'*int_month_timestamp';


  # XXX - Because of https://github.com/irods/irods/issues/6918
  # Intended to use *FlagObj = ""; msiAddKeyValToMspStr("irodsAdminRmTrash", "", *FlagObj);
  # Instead, setting the *FlagObj = 'irodsAdminRmTrash=' without using msiAddKeyValToMspStr
  # Applying the same logic for *FlagColl below
  *FlagObj = 'irodsAdminRmTrash='

  # The results are sorted in reverse order to ensure a subcollection with a
  # timestamp is deleted before its parent, which also has a timestamp. If the
  # parent were deleted before the child was attempted to be deleted, the child
  # delete call would fail, logging an error and causing the the trash removal
  # run to fail. This happens, because the call to delete the parent also
  # deletes the child. Sort the results by collection path in descending order,
  # lists a child collection before its parent.
  foreach(*Row in SELECT META_COLL_ATTR_VALUE, ORDER_DESC(COLL_NAME)
                    WHERE COLL_NAME like '/*zone/trash/%'
                      AND META_COLL_ATTR_NAME = 'ipc::trash_timestamp'
                        AND META_COLL_ATTR_VALUE <= *month_timestamp) {
                          *ts = *Row.META_COLL_ATTR_VALUE;
                          *rowCollName = *Row.COLL_NAME;
                          *status = errorcode(msiRmColl(*rowCollName, *FlagObj, *Status));
                          if (*status == 0) {
                            writeLine(
                              "serverLog",
                              "DS: Removed trash collection - *rowCollName with trash timestamp - *ts" );
                          } else {
                            writeLine("serverLog", "DS: Unable to remove trash collection - *rowCollName, error code returned *status");
                            *verdict = false;
                          }
  }

  foreach(*Row in SELECT META_DATA_ATTR_VALUE, DATA_NAME, COLL_NAME
                    WHERE COLL_NAME like '/*zone/trash/%'
                      AND META_DATA_ATTR_NAME = 'ipc::trash_timestamp'
                        AND META_DATA_ATTR_VALUE <= *month_timestamp) {
    *ts = *Row.META_DATA_ATTR_VALUE;
    *rowCollName = *Row.COLL_NAME;
    *rowDataName = *Row.DATA_NAME;
    *absDataPath = *rowCollName ++ "/" ++ *rowDataName;
    *FlagColl = 'irodsAdminRmTrash='
    msiAddKeyValToMspStr("objPath", *absDataPath, *FlagColl);
    *status = errorcode(msiDataObjUnlink(*FlagColl, *Status));
    if (*status == 0) {
      writeLine(
        "serverLog", "DS: Removed trash data object - *absDataPath with trash timestamp - *ts" );
    } else {
      writeLine(
        "serverLog",
        "DS: Unable to remove trash data object - *absDataPath, error code returned *status" );
      *verdict = false;
    }
  }

  if (*verdict) {
    *subject = ipc_ZONE ++ ' trash removal succeeded';
    *body = 'SSIA';
  } else {
    *subject = ipc_ZONE ++ ' trash removal failed';
    *body = 'View the irods logs for details';
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
