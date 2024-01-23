# This is a library of rules for periodic tasks like updating quota usage data.
#
# © 2023 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

_cyverse_housekeeping_schedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
	writeLine('serverLog', 'DS: scheduling *Desc');
# XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See
#       https://github.com/irods/irods/issues/5413.
# 	eval(``delay('<PLUSET>0s</PLUSET><EF>*Freq</EF>') {`` ++ *RuleName ++ ``}`` );
	eval(
		``delay(
			'<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
			<PLUSET>0s</PLUSET>
			<EF>*Freq</EF>'
		) {`` ++ *RuleName ++ ``}`` );
# XXX - ^^^
}

_cyverse_housekeeping_reschedulePeriodicPolicy(*RuleName, *Freq, *Desc) {
	*scheduled = false;
	foreach(*row in SELECT RULE_EXEC_ID, RULE_EXEC_FREQUENCY WHERE RULE_EXEC_NAME = '*RuleName') {
		if (*scheduled || *row.RULE_EXEC_FREQUENCY != *Freq) {
			writeLine('serverLog', 'DS: unscheduling *Desc');
			*idArg = execCmdArg(*row.RULE_EXEC_ID);
			*status = errorcode(
				msiExecCmd('delete-scheduled-rule', *idArg, 'null', 'null', 'null', *out) );
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
		_cyverse_housekeeping_schedulePeriodicPolicy(*RuleName, *Freq, *Desc);
		writeLine('stdout', 'scheduled *Desc');
	}
}


#
# QUOTAS
#

# This rule updates the storage usage of every user. It is safe to be run
# asynchronously.
#
cyverse_housekeeping_updateQuotaUsage {
	writeLine('serverLog', 'DS: updating quota usage');
	if (0 == errormsg(msiQuota, *msg)) {
		writeLine('serverLog', 'DS: quota usage updated');
	} else {
		writeLine('serverLog', "DS: quota usage update failed: *msg");
	}
}

# This rule schedules the hourly calculation of quota usage data. If it
# reschedules the calculation, it writes 'scheduled quota usage updates' to
# standard output. If it doesn't error out, but doesn't reschedule the
# calculation, it writes 'quota usage updates already scheduled'.
#
cyverse_housekeeping_rescheduleQuotaUsageUpdate {
	_cyverse_housekeeping_reschedulePeriodicPolicy(
		``cyverse_housekeeping_updateQuotaUsage``, '1h REPEAT FOR EVER', 'quota usage updates' );
}


#
# STORAGE FREE SPACE
#

# NOTE: This runs on the resource server hosting the resource whose free space
# is in question.
_cyverse_housekeeping_determineStorageFreeSpace(*Host, *RescName) {
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

# This rule updates the catalog information on the amount of free space exists
# in each resource. It is safe to be run asynchronously.
#
cyverse_housekeeping_determineAllStorageFreeSpace {
	writeLine('serverLog', 'DS: determining free space on resource servers');
	foreach(*record in
		SELECT RESC_LOC, RESC_NAME WHERE RESC_TYPE_NAME = 'unixfilesystem' AND RESC_STATUS = 'up'
	) {
		*host = *record.RESC_LOC;
		*resc = *record.RESC_NAME;
		if (0 > errormsg(_cyverse_housekeeping_determineStorageFreeSpace(*host, *resc), *msg)) {
			writeLine('serverLog', "DS: failed to determine free space on *host for *resc: *msg");
		}
	}
	writeLine('serverLog', 'DS: determined free space on resource servers');
}

# This rule schedules the daily determination of the available disk space for
# all Unix file system resources. If it reschedules the determination, it writes
# 'scheduled storage determination' to standard output. If it doesn't error out,
# but it doesn't reschedule the determination, it writes 'storage determination
# already scheduled'.
#
cyverse_housekeeping_rescheduleStorageFreeSpaceDetermination {
	_cyverse_housekeeping_reschedulePeriodicPolicy(
		``cyverse_housekeeping_determineAllStorageFreeSpace``,
		'1d REPEAT FOR EVER',
		'storage determination' );
}


#
# TRASH REMOVAL
#

_cyverse_housekeeping_rmTrashColl(*CutOffTimestamp, *SUCCEEDED) {
	*SUCCEEDED = true;
	*zone = cyverse_ZONE;
# XXX - Because of https://github.com/irods/irods/issues/6918
# 	*rmOpts = "";
# 	msiAddKeyValToMspStr("irodsAdminRmTrash", "", *rmOpts);
	*rmOpts = 'irodsAdminRmTrash='
# XXX - ^^^

	# The results are sorted in reverse order to ensure a subcollection with a
	# timestamp is deleted before its parent, which also has a timestamp. If the
	# parent were deleted before the child was attempted to be deleted, the
	# child delete call would fail, logging an error and causing the the trash
	# removal run to fail. This happens, because the call to delete the parent
	# also deletes the child. Sort the results by collection path in descending
	# order, lists a child collection before its parent.
	foreach( *row in
		SELECT META_COLL_ATTR_VALUE, ORDER_DESC(COLL_NAME)
		WHERE COLL_NAME like '/*zone/trash/%'
			AND META_COLL_ATTR_NAME = 'ipc::trash_timestamp'
			AND META_COLL_ATTR_VALUE <= *CutOffTimestamp
	) {
		*ts = *row.META_COLL_ATTR_VALUE;
		*rowCollName = *row.COLL_NAME;
		*status = errorcode(msiRmColl(*rowCollName, *rmOpts, *_));
		if (*status == 0) {
			writeLine(
				"serverLog", "DS: Removed trash collection - *rowCollName with trash timestamp - *ts" );
		} else {
			writeLine(
				"serverLog",
				"DS: Unable to remove trash collection - *rowCollName, error code returned *status" );
			*SUCCEEDED = false;
		}
	}
}

_cyverse_housekeeping_rmTrashData(*CutOffTimestamp, *SUCCEEDED) {
	*SUCCEEDED = true;
	*zone = cyverse_ZONE;
	foreach( *row in
		SELECT META_DATA_ATTR_VALUE, DATA_NAME, COLL_NAME
		WHERE COLL_NAME like '/*zone/trash/%'
			AND META_DATA_ATTR_NAME = 'ipc::trash_timestamp'
			AND META_DATA_ATTR_VALUE <= *CutOffTimestamp
	) {
		*ts = *row.META_DATA_ATTR_VALUE;
		*rowCollName = *row.COLL_NAME;
		*rowDataName = *row.DATA_NAME;
		*absDataPath = *rowCollName ++ "/" ++ *rowDataName;
# XXX - Because of https://github.com/irods/irods/issues/6918
# 		*rmOpts = "";
#		msiAddKeyValToMspStr("irodsAdminRmTrash", "", *rmOpts);
		*rmOpts = 'irodsAdminRmTrash='
# XXX - ^^^
		msiAddKeyValToMspStr("objPath", *absDataPath, *rmOpts);
		*status = errorcode(msiDataObjUnlink(*rmOpts, *_));
		if (*status == 0) {
			writeLine(
				"serverLog",
				"DS: Removed trash data object - *absDataPath with trash timestamp - *ts" );
		} else {
			writeLine(
				"serverLog",
				"DS: Unable to remove trash data object - *absDataPath, error code returned *status" );
			*SUCCEEDED = false;
		}
	}
}

# This rule deletes all collections and data objects that have the
# ipc::trash_timestamp AVU set to a time that is at least 30 days in the past.
# It sends an email indicating whether or not it succeeded. It is safe to be run
# asynchronously.
#
cyverse_housekeeping_rmTrash {
	writeLine('serverLog', 'DS: starting trash removal');
	msiGetSystemTime(*timestamp, "");

	# 2,592,000 is the number of seconds in 30 days. We subtract this value from
	# the current timestamp to calculate the threshold time for items in the
	# trash that are older than 30 days.
	*intMonthTimestamp = int(*timestamp) - 2592000;

	# iRODS appends a leading 0 to epoch timestamps, but the int conversion
	# removes it. To enable string comparison done below, we add a leading 0 to
	# the month_timestamp string.
	*monthTimestamp = '0'++'*intMonthTimestamp';

	_cyverse_housekeeping_rmTrashColl(*monthTimestamp, *rmCollSuccess);
	_cyverse_housekeeping_rmTrashData(*monthTimestamp, *rmDataSuccess);
	if (*rmCollSuccess && *rmDataSuccess) {
		*subject = cyverse_ZONE ++ ' trash removal succeeded';
		*body = 'SSIA';
	} else {
		*subject = ipc_ZONE ++ ' trash removal failed';
		*body = 'View the irods logs for details';
	}
	if (0 != errorcode(msiSendMail(cyverse_REPORT_EMAIL_ADDR, *subject, *body))) {
		writeLine('serverLog', 'DS: failed to mail trash removal report');
	}
	writeLine('serverLog', 'DS: completed trash removal');
}

# This rule schedules the weekly trash removal. If it reschedules the removal,
# it writes 'scheduled trash removal' to standard output. If it doesn't error
# out, but it doesn't reschedule the removal, it writes 'trash removal already
# scheduled'.
#
cyverse_housekeeping_rescheduleTrashRemoval {
	_cyverse_housekeeping_reschedulePeriodicPolicy(
		``cyverse_housekeeping_rmTrash``, '7d REPEAT FOR EVER', 'trash removal' );
}
