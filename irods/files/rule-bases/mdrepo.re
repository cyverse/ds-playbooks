# MD Repo project policy
#
# XXX - This works around a bug in tickets that prevents a new data object from  being created in a
#       folder using a write ticket. Before an attempt is made to create a data object using a
#       ticket, an empty data object is created as the ticket author. Since the data object exists,
#       as long as the operation creating the data object allows overwriting an existing data
#       object, the operation will succeed using the ticket for authorization. After upgrading to
#       4.2.12+, these rules should be removed.

@include 'mdrepo-env'

_mdrepo_getValue(*KVMap, *Key) = if errorcode(*KVMap.'*Key') == 0 then *KVMap.'*Key' else ''

_mdrepo_forMDRepo(*Path) = mdrepo_LANDING_COLL != '' && *Path like mdrepo_LANDING_COLL ++ '/*'

_mdrepo_collExists(*Coll) =
	let *exists = false in
	let *_ = foreach(*rec in SELECT COLL_ID WHERE COLL_NAME = *Coll) {
			*exists = true
		} in
	*exists

_mdrepo_obj_exists(*DataPath) =
	let *exists = false in
	let *collPath = '' in
	let *dataName = '' in
	let *_ = msiSplitPath(*DataPath, *collPath, *dataName) in
	let *_ = foreach(*rec in SELECT DATA_ID WHERE COLL_NAME = *collPath AND DATA_NAME = *dataName) {
			*exists = true
		} in
	*exists

_mdrepo_usingTicket() = _mdrepo_getValue(temporaryStorage, 'mdrepo_ticket') != ''

_mdrepo_needEnsureExists(*Entity) = _mdrepo_forMDRepo(*Entity) && _mdrepo_usingTicket()

_mdrepo_needEnsureObjExists(*DataObj) =
	_mdrepo_needEnsureExists(*DataObj) && ! _mdrepo_obj_exists(*DataObj)

_mdrepo_logMsg(*Msg) {
	writeLine('serverLog', 'MD REPO: *Msg');
}

_mdrepo_touchDataObj(*DataObj) {
	_mdrepo_logMsg('creating empty data object *DataObj');
	*svcAcntArg = execCmdArg(mdrepo_SVC_ACCOUNT);
	*dataObjArg = execCmdArg(*DataObj);
	*args = "*svcAcntArg *dataObjArg";
	*err = errormsg(msiExecCmd('md-repo-touch-obj', "*args", cyverse_RE_HOST, "", "", *out), *msg);

	if (*err != 0) {
		*_ = errorcode(msiGetStderrInExecCmdOut(*out, *cmdErr));
		_mdrepo_logMsg('failed to create data object *DataObj: *msg (*cmdErr)');
		*err;
	}
}

mdrepo_api_coll_create_pre(*Instance, *Comm, *CollCreateInp) {
	if (
		_mdrepo_needEnsureExists(*CollCreateInp.coll_name)
		&& ! _mdrepo_collExists(*CollCreateInp.coll_name)
	) {
		_mdrepo_logMsg('creating collection ' ++ *CollCreateInp.coll_name);
		*svcAcntArg = execCmdArg(mdrepo_SVC_ACCOUNT);
		*collArg = execCmdArg(*CollCreateInp.coll_name);
		*args = "*svcAcntArg *collArg";
		*err = errormsg(msiExecCmd('md-repo-mkdir', "*args", cyverse_RE_HOST, "", "", *out), *msg);

		if (*err != 0) {
			*_ = errorcode(msiGetStderrInExecCmdOut(*out, *cmdErr));
			_mdrepo_logMsg(
				'failed to create collection ' ++ *CollCreateInp.coll_name ': ' ++ *msg
				++ ' (' ++ *cmdErr ++ ')' );
			*err;
		}
	}
}

mdrepo_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
	if (_mdrepo_needEnsureObjExists(*DataObjInp.obj_path)) {
		_mdrepo_touchDataObj(*DataObjInp.obj_path);
	}
}

mdrepo_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PortalOprOut) {
	if (_mdrepo_needEnsureObjExists(*DataObjInp.obj_path)) {
		_mdrepo_touchDataObj(*DataObjInp.obj_path);
	}
}

mdrepo_database_mod_ticket_post(
	*Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5
) {
	temporaryStorage.mdrepo_ticket = *TicketString;
}
