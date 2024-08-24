# VERSION 12
#
# bisque.re
#
# BisQue related rules

@include 'bisque-env'

_bisque_COLL = 'bisque_data'

_bisque_ID_ATTR = 'ipc-bisque-id'

_bisque_URI_ATTR = 'ipc-bisque-uri'

_bisque_USER = 'bisque'

_bisque_stripTrailingSlash(*Path) = if str(*Path) like '*/' then trimr(str(*Path), '/') else *Path

_bisque_determineSrc(*BaseSrcColl, *BaseDestColl, *DestEntity) =
	let *dest = _bisque_stripTrailingSlash(*DestEntity) in
	_bisque_stripTrailingSlash(*BaseSrcColl)
		++ substr(*dest, strlen(_bisque_stripTrailingSlash(*BaseDestColl)), strlen(*dest))

_bisque_getHomeUser(*Path) =
	if *Path like regex '^/' ++ cyverse_ZONE ++ '/home/shared($|/.*)' then ''
	else if *Path like regex '^/' ++ cyverse_ZONE ++ '/home/[^/]+/.+' then elem(split(*Path, '/'), 2)
	else
		if *Path like regex '^/' ++ cyverse_ZONE ++ '/trash/home/[^/]+/.+'
		then elem(split(*Path, '/'), 3)
	else ''

_bisque_getClient(*AuthorName, *AuthorZone, *Path) =
	let *homeUser = _bisque_getHomeUser(*Path) in
	if *homeUser != '' then *homeUser
	else if *AuthorZone == cyverse_ZONE then *AuthorName
	else ''

_bisque_isInBisque(*CollPath, *DataObjName) =
	let *idAttr = _bisque_ID_ATTR in
	let *status =
	foreach ( *reg in
			SELECT count(META_DATA_ATTR_VALUE)
			WHERE COLL_NAME = "*CollPath"
				AND DATA_NAME = "*DataObjName"
				AMD META_DATA_ATTR_NAME = "*idAttr"
		) { *found = *reg.META_DATA_ATTR_VALUE != '0'; } in
	*found

_bisque_isInProj(*Proj, *Path) = *Path like '/' ++ cyverse_ZONE ++ "/home/shared/*Proj/*"

_bisque_isInProjs(*Projs, *Path) =
	if size(*Projs) == 0 then false
	else if _bisque_isInProj(hd(*Projs), *Path) then true
	else _bisque_isInProjs(tl(*Projs), *Path)

_bisque_isForBisque(*Author, *Path) =
	*Author != _bisque_USER
	&& (cyverse_isForSvc(_bisque_USER, _bisque_COLL, /*Path)
		|| _bisque_isInProjs(bisque_PROJECTS, *Path))

_bisque_isUser(*Client) =
	if *Client == '' then false
	else
		let *zone = cyverse_ZONE in
		let *ans = false in
		let *_ = foreach ( *_ in
				SELECT USER_TYPE
				WHERE USER_NAME = *Client AND USER_ZONE = *zone AND USER_TYPE = 'rodsuser'
			) { *ans = true } in
		*ans

_bisque_logMsg(*Msg) {
	writeLine('serverLog', "BISQUE: *Msg");
}

_bisque_mkIrodsUrl(*Path) =
	let *pathArg = execCmdArg(*Path) in
	let *status = errorcode(
			msiExecCmd('url-encode-path-segments.sh', *pathArg, cyverse_RE_HOST, '', '', *out)
		) in
	if *status != 0 then
		let *_ = msiGetStderrInExecCmdOut(*out, *resp) in
		let *_ = _bisque_logMsg("FAILURE - failed to encode path (*resp)") in
		''
	else
		let *_ = msiGetStdoutInExecCmdOut(*out, *encodedPath) in
		bisque_IRODS_URL_BASE ++ *encodedPath

# Tells BisQue to create a link for a given user to a data object.
#
# bisque_paths.py (--alias user) ln -P permission /path/to/data.object
#
bisque_ln(*Perm, *Client, *Path) {
	if (*Client != '') {
		_bisque_logMsg("linking *Path for *Client with permission *Perm");
	} else {
		_bisque_logMsg("linking *Path with permission *Perm");
	}

	*url = _bisque_mkIrodsUrl(*Path);
	*aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
	*pArg = execCmdArg(*Perm);
	*pathArg = execCmdArg(*url);
	*argStr = "*aliasOpt ln -P *pArg *pathArg";
	*status = errorcode(msiExecCmd('bisque_paths.py', *argStr, cyverse_RE_HOST, '', '', *out));

	if (*status != 0) {
		msiGetStderrInExecCmdOut(*out, *resp);
		_bisque_logMsg("FAILURE - *resp");
		if (*Client != '') {
			_bisque_logMsg("failed to link *Path for *Client with permission *Perm");
		} else {
			_bisque_logMsg("failed to link *Path with permission *Perm");
		}
		fail;
	} else {
		msiGetStdoutInExecCmdOut(*out, *resp);
		*props = split(trimr(triml(*resp, ' '), '/'), ' ')
		msiStrArray2String(*props, *kvStr);
		msiString2KeyValPair(*kvStr, *kvs);
		msiGetValByKey(*kvs, 'resource_uniq', *qId);
		*id = substr(*qId, 1, strlen(*qId) - 1);
		msiGetValByKey(*kvs, 'uri', *qUri);
		*uri = substr(*qUri, 1, strlen(*qUri) - 1);
		cyverse_setProtectedAVU(*Path, _bisque_ID_ATTR, *id, '');
		cyverse_setProtectedAVU(*Path, _bisque_URI_ATTR, *uri, '');

		if (*Client != '') {
			_bisque_logMsg("linked *Path for *Client with permission *Perm");
		} else {
			_bisque_logMsg("linked *Path with permission *Perm");
		}
	}
}

# Tells BisQue to change the path of a linked data object.
#
# bisque_paths.py (--alias user) mv /old/path/to/data.object /new/path/to/data.object
#
_bisque_mv(*Client, *OldPath, *NewPath) {
	if (*Client != '') {
		_bisque_logMsg("moving link from *OldPath to *NewPath for *Client");
	} else {
		_bisque_logMsg("moving link from *OldPath to *NewPath");
	}

	*oldUrl = _bisque_mkIrodsUrl(*OldPath);
	*newUrl = _bisque_mkIrodsUrl(*NewPath);
	*aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
	*oldPathArg = execCmdArg(*oldUrl);
	*newPathArg = execCmdArg(*newUrl);
	*argStr = "*aliasOpt mv *oldPathArg *newPathArg";
	*status = errorcode(msiExecCmd('bisque_paths.py', *argStr, cyverse_RE_HOST, '', '', *out));

	if (*status != 0) {
		msiGetStderrInExecCmdOut(*out, *resp);
		_bisque_logMsg("FAILURE - *resp");

		if (*Client != '') {
			_bisque_logMsg("failed to move link from *OldPath to *NewPath for *Client");
		} else {
			_bisque_logMsg("failed to move link from *OldPath to *NewPath");
		}
		fail;
	} else {
		if (*Client != '') {
			_bisque_logMsg("moved link from *OldPath to *NewPath for *Client");
		} else {
			_bisque_logMsg("moved link from *OldPath to *NewPath");
		}
	}
}

# Tells BisQue to remove a link to data object.
#
# bisque_paths.py (--alias user) rm /path/to/data.object
#
_bisque_rm(*Client, *Path) {
	if (*Client != '') {
		_bisque_logMsg("removing link from *Path for *Client");
	} else {
		_bisque_logMsg("removing link from *Path");
	}

	*url = _bisque_mkIrodsUrl(*Path);
	*aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
	*pathArg = execCmdArg(*url);
	*argStr = "*aliasOpt rm *pathArg";
	*status = errorcode(msiExecCmd('bisque_paths.py', *argStr, cyverse_RE_HOST, '', '', *out));

	if (*status != 0) {
		msiGetStderrInExecCmdOut(*out, *resp);
		_bisque_logMsg("FAILURE - *resp");
		if (*Client != '') {
			_bisque_logMsg("failed to remove link to *Path for *Client");
		} else {
			_bisque_logMsg("failed to remove link to *Path");
		}
		fail;
	} else {
		if (*Client != '') {
			_bisque_logMsg("removed link to *Path for *Client");
		} else {
			_bisque_logMsg("removed link to *Path");
		}
	}
}

_bisque_schedLn(*Perm, *Client, *Path) {
	if (*Client != '') {
		_bisque_logMsg("scheduling linking of *Path for *Client with permission *Perm");
	} else {
		_bisque_logMsg("scheduling linking of *Path with permission *Perm");
	}
# XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See
#       https://github.com/irods/irods/issues/5413.
# 	delay('<PLUSET>1s</PLUSET>') {bisque_ln(*Perm, *Client, *Path)};
	delay(
		' <INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
			<PLUSET>1s</PLUSET> '
	) {bisque_ln(*Perm, *Client, *Path)};
# XXX - ^^^
}

_bisque_handleNewObj(*Client, *Path) {
	cyverse_giveAccessDataObj(_bisque_USER, 'write', *Path);
	*perm = if _bisque_isInProjs(bisque_PROJECTS, *Path) then 'published' else 'private';
	_bisque_schedLn(*perm, *Client, *Path);
}

_bisque_handleObjCreate(*CreatorName, *CreatorZone, *Path) {
	if (_bisque_isForBisque(*CreatorName, *Path)) {
		_bisque_handleNewObj(_bisque_getClient(*CreatorName, *CreatorZone, *Path), *Path);
	}
}

# Add a call to this rule from inside the acPostProcForCollCreate PEP.
bisque_acPostProcForCollCreate {
	if (_bisque_isForBisque($userNameClient, $collName)) {
		cyverse_giveAccessColl(_bisque_USER, 'write', $collName);
	}
}

# Add a call to this rule from inside the acPostProcForObjRename PEP.
bisque_acPostProcForObjRename(*SrcEntity, *DestEntity) {
	*client = _bisque_getClient($userNameClient, $rodsZoneClient, *SrcEntity);
	*forBisque = _bisque_isForBisque($userNameClient, *DestEntity);
	*type = cyverse_getEntityType(*DestEntity);

	if (cyverse_isColl(*type)) {
		if (*forBisque) {
			cyverse_giveAccessColl(_bisque_USER, 'write', *DestEntity);
		}

		foreach (*collPat in list(*DestEntity, "*DestEntity/%")) {
			foreach (*rec in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE "*collPat") {
				*collPath = *rec.COLL_NAME;
				*dataObjName = *rec.DATA_NAME;

				if (_bisque_isInBisque(*collPath, *dataObjName)) {
					*srcSubColl = _bisque_determineSrc(*SrcEntity, *DestEntity, *collPath);
					_bisque_mv(*client, "*srcSubColl/*dataObjName", "*collPath/*dataObjName");
				} else if (*forBisque) {
					_bisque_handleNewObj(*client, "*collPath/*dataObjName");
				}
			}
		}
	} else if (cyverse_isDataObj(*type)) {
		msiSplitPath(*DestEntity, *collPath, *dataObjName);

		if (_bisque_isInBisque(*collPath, *dataObjName)) {
			_bisque_mv(*client, *SrcEntity, *DestEntity);
		} else if (*forBisque) {
			_bisque_handleNewObj(*client, *DestEntity);
		}
	}
}

# Add a call to this rule from inside the acDataDeletePolicy PEP.
bisque_acDataDeletePolicy {
	msiSplitPath($objPath, *collPath, *dataObjName);

	temporaryStorage."bisque_$objPath" =
		if _bisque_isInBisque(*collPath, *dataObjName) then 'rm' else ''
}

# Add a call to this rule from inside the acPostProcForDelete PEP.
bisque_acPostProcForDelete {
	if (temporaryStorage."bisque_$objPath" == 'rm') {
		_bisque_rm(_bisque_getClient($userNameClient, $rodsZoneClient, $objPath), $objPath);
	}
}

bisque_dataObjCreated(*User, *Zone, *DataObjInfo) {
	_bisque_handleObjCreate(*User, *Zone, *DataObjInfo.logical_path);
}
