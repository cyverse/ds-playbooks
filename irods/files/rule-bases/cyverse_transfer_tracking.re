_cyverse_transfer_tracking_addTransfer(*User, *Dir, *Vol) {
	*query = "select USER_ID where USER_NAME = '*User'";
 	msiExecStrCondQuery(*query, *res);

	foreach(*res) {
		*userId = *res.USER_ID;
	}

	*userArg = execCmdArg(*userId);
	*dirArg = execCmdArg(*Dir);
	*volArg = execCmdArg(*Vol);
  	*args = "*userArg *dirArg *volArg";
	*ec = errormsg(msiExecCmd("add-transfer", *args, "null", "null", "null", *result), *err);
	if (*ec != 0) {
		msiGetStderrInExecCmdOut(*result, *resp);
		writeLine('serverLog', "add-transfer failed: *resp");
		failmsg(*ec, *resp);
	}
}


cyverse_transfer_tracking_api_data_obj_write_post(*Instance, *Comm, *DataObjWriteInp, *Buf) {
	*rc = errormsg(
		_cyverse_transfer_tracking_addTransfer(*Comm.user_user_name, 'in', *DataObjWriteInp.len),
		*err );

	if (*rc != 0) {
		*msg = "_cyverse_transfer_tracking_addTransfer("
			++ *Comm.user_user_name ++ ", 'in', " ++ *DataObjWriteInp.len
			++ ") failed: " ++ *err;

		writeLine('serverLog', *msg);
	}
}

cyverse_transfer_tracking_api_data_obj_read_post(*Instance, *Comm, *DataObjReadInp, *Buf) {
	*rc = errormsg(
		_cyverse_transfer_tracking_addTransfer(*Comm.user_user_name, 'out', *DataObjReadInp.len),
		*err );

	if (*rc != 0) {
		*msg = "_cyverse_transfer_tracking_addTransfer("
			++ *Comm.user_user_name ++ ", 'out', " ++ *DataObjReadInp.len
			++ ") failed: " ++ *err;

		writeLine('serverLog', *msg);
	}
}

cyverse_transfer_tracking_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *Buf, *PORTAL_OPR) {
	*rc = errormsg(
		_cyverse_transfer_tracking_addTransfer(*Comm.user_user_name, 'in', *DataObjInp.data_size),
		*err );

	if (*rc != 0) {
		*msg = "_cyverse_transfer_tracking_addTransfer("
			++ *Comm.user_user_name ++ ", 'in', " ++ *DataObjInp.data_size
			++ ") failed: " ++ *err;

		writeLine('serverLog', *msg);
	}
}

cyverse_transfer_tracking_api_data_obj_get_post(*Instance, *Comm, *DataObjInp, *Buf, *PORTAL_OPR) {
	*rc = errormsg(
		_cyverse_transfer_tracking_addTransfer(*Comm.user_user_name, 'out', *DataObjInp.data_size),
		*err );

	if (*rc != 0) {
		*msg = "_cyverse_transfer_tracking_addTransfer("
			++ *Comm.user_user_name ++ ", 'out', " ++ *DataObjInp.data_size
			++ ") failed: " ++ *err;

		writeLine('serverLog', *msg);
	}
}

cyverse_transfer_tracking_api_bulk_data_obj_put_post(*Instance, *Comm, *BulkOprInp, *Buf) {
	# bulk can hold up to 50 files, with sizes in data_size_0 through
	# data_size_49. walk through and sum the sizes

	*totalVol = 0;

	foreach(*k in *BulkOprInp) {
		if (*k like "data_size_*") then {
			*totalVol = *totalVol + int(*BulkOprInp.*k);
		}
	}

	if (
		errormsg(_cyverse_transfer_tracking_addTransfer(*Comm.user_user_name, 'in', *totalVol), *err)
		!= 0
	) {
		*msg = "_cyverse_transfer_tracking_addTransfer("
			++ *Comm.user_user_name ++ ", 'in', " ++ *totalVol
			++ ") failed: " ++ *err;

		writeLine('serverLog', *msg);
	}
}