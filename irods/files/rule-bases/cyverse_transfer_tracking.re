_cyverse_transfer_tracking_addTransfer(*User, *Zone, *Dir, *Vol) {
	*query = \
		"select USER_ID where USER_NAME = '*User' and USER_ZONE = '*Zone' and USER_TYPE = 'rodsuser'";

	msiExecStrCondQuery(*query, *res);

	foreach( *res in
		select USER_ID where USER_NAME = '*User' and USER_ZONE = '*Zone' and USER_TYPE = 'rodsuser'
	) {
		*userArg = execCmdArg(*res.USER_ID);
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
}


cyverse_transfer_tracking_api_data_obj_write_post(*Instance, *Comm, *DataObjWriteInp, *Buf) {
	*user = *Comm.user_user_name;
	*zone = *Comm.user_rods_zone;
	*vol = *DataObjWriteInp.len;

	if (errormsg(_cyverse_transfer_tracking_addTransfer(*user, *zone, 'in', *vol), *err) != 0) {
		writeLine(
			'serverLog',
			"_cyverse_transfer_tracking_addTransfer(*user, *zone, in, *vol) failed: *err" );
	}
}

cyverse_transfer_tracking_api_data_obj_read_post(*Instance, *Comm, *DataObjReadInp, *Buf) {
	*user = *Comm.user_user_name;
	*zone = *Comm.user_rods_zone;
	*vol = *DataObjReadInp.len;

	if (errormsg(_cyverse_transfer_tracking_addTransfer(*user, *zone, 'out', *vol), *err) != 0) {
		writeLine(
			'serverLog',
			"_cyverse_transfer_tracking_addTransfer(*user, *zone, out, *vol) failed: *err" );
	}
}

cyverse_transfer_tracking_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *Buf, *PORTAL_OPR) {
	*user = *Comm.user_user_name;
	*zone = *Comm.user_rods_zone;
	*vol = *DataObjInp.data_size;

	if (errormsg(_cyverse_transfer_tracking_addTransfer(*user, *zone, 'in', *vol), *err) != 0) {
		writeLine(
			'serverLog',
			"_cyverse_transfer_tracking_addTransfer(*user, *zone, in, *vol) failed: *err" );
	}
}

cyverse_transfer_tracking_api_data_obj_get_post(*Instance, *Comm, *DataObjInp, *Buf, *PORTAL_OPR) {
	*user = *Comm.user_user_name;
	*zone = *Comm.user_rods_zone;
	*vol = *DataObjInp.data_size;

	if (errormsg(_cyverse_transfer_tracking_addTransfer(*user, *zone, 'out', *vol), *err) != 0) {
		writeLine(
			'serverLog',
			"_cyverse_transfer_tracking_addTransfer(*user, *zone, out, *vol) failed: *err" );
	}
}

cyverse_transfer_tracking_api_bulk_data_obj_put_post(*Instance, *Comm, *BulkOprInp, *Buf) {
	# bulk can hold up to 50 files, with sizes in data_size_0 through
	# data_size_49. walk through and sum the sizes

	*user = *Comm.user_user_name;
	*zone = *Comm.user_rods_zone;
	*totalVol = 0;

	foreach(*k in *BulkOprInp) {
		if (*k like "data_size_*") {
			*totalVol = *totalVol + int(*BulkOprInp.*k);
		}
	}

	if (errormsg(_cyverse_transfer_tracking_addTransfer(*user, *zone, 'in', *totalVol), *err) != 0) {
		writeLine(
			'serverLog',
			"_cyverse_transfer_tracking_addTransfer(*user, *zone, in, *totalVol) failed: *err" );
	}
}