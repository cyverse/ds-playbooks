# MD Repo project policy
#
# XXX - This works around bugs in tickets that prevent a data object from being
#       created in a collection using a write ticket. Before an attempt is made
#       to create a data object using a ticket, anempty data object is created
#       as the ticket owner. Since the data object exists, as long as the
#       operation creating it allows overwriting, the operation will succeed
#       using the ticket for authorization. After upgrading to 4.2.12+, this
#       rulebase should be removed.
#
# © 2023 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

@include 'mdrepo-env'

_mdrepo_getValue(*KVMap, *Key) = if errorcode(*KVMap.'*Key') == 0 then *KVMap.'*Key' else ''

_mdrepo_inColl(*Coll, *Path) = *Path like *Coll ++ '/*'

_mdrepo_inColls(*Colls, *Path) =
	if size(*Colls) == 0 then false
	else if _mdrepo_inColl(hd(*Colls), *Path) then true
	else _mdrepo_inColls(tl(*Colls), *Path)

_mdrepo_forMDRepo(*Path) = _mdrepo_inColls(mdrepo_LANDING_COLLS, *Path)

_mdrepo_dataObjExists(*DataPath) =
	let *exists = false in
	let *collPath = '' in
	let *dataName = '' in
	let *_ = msiSplitPath(*DataPath, *collPath, *dataName) in
	let *_ = foreach(*rec in SELECT DATA_ID WHERE COLL_NAME = *collPath AND DATA_NAME = *dataName) {
			*exists = true
		} in
	*exists

_mdrepo_getTicketOwner(*Ticket) =
	let *owner = '' in
	let *_ = foreach(*rec in SELECT TICKET_OWNER_NAME where TICKET_STRING = *Ticket) {
			*owner = *rec.TICKET_OWNER_NAME;
		} in
	*owner

_mdrepo_logMsg(*Msg) {
	writeLine('serverLog', 'MD REPO: *Msg');
}

_mdrepo_ensureMdRepoObjExists(*DataObj, *Ticket) {
	if (
		*Ticket != ''
		&& _mdrepo_forMDRepo(*DataObj)
		&& ! _mdrepo_dataObjExists(*DataObj)
	) {
		_mdrepo_logMsg('creating empty data object *DataObj');
		*svcAcntArg = execCmdArg(_mdrepo_getTicketOwner(*Ticket));
		*dataObjArg = execCmdArg(*DataObj);
		*args = "*svcAcntArg *dataObjArg";
		*err = errormsg(
			msiExecCmd('md-repo-touch-obj', "*args", cyverse_RE_HOST, "", "", *out), *msg );

		if (*err != 0) {
			*_ = errorcode(msiGetStderrInExecCmdOut(*out, *cmdErr));
			_mdrepo_logMsg('failed to create data object *DataObj: *msg (*cmdErr)');
			*err;
		}
	}
}

mdrepo_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
	_mdrepo_ensureMdRepoObjExists(
		*DataObjInp.obj_path, _mdrepo_getValue(temporaryStorage, 'mdrepo_ticket') );
}

mdrepo_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PortalOprOut) {
	_mdrepo_ensureMdRepoObjExists(
		*DataObjInp.obj_path, _mdrepo_getValue(temporaryStorage, 'mdrepo_ticket') );
}

mdrepo_database_mod_ticket_post(
	*Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5
) {
	temporaryStorage.mdrepo_ticket = *TicketString;
}
