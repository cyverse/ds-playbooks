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

_bisque_isInBisque(*CollName, *DataName) =
  let *idAttr = _bisque_ID_ATTR in
  let *status =
    foreach (*reg in select count(META_DATA_ATTR_VALUE)
                     where COLL_NAME = '*CollName'
                       and DATA_NAME = '*DataName'
                       and META_DATA_ATTR_NAME = '*idAttr') {
      *found = *reg.META_DATA_ATTR_VALUE != '0';
    }
  in *found

_bisque_isInProject(*Project, *Path) = *Path like '/' ++ cyverse_ZONE ++ '/home/shared/*Project/*'

_bisque_isInProjects(*Projects, *Path) =
  if size(*Projects) == 0
  then false
  else if _bisque_isInProject(hd(*Projects), *Path)
       then true
       else _bisque_isInProjects(tl(*Projects), *Path)

_bisque_isForBisque(*Author, *Path) =
  *Author != _bisque_USER
  && (cyverse_isForSvc(_bisque_USER, _bisque_COLL, /*Path)
      || _bisque_isInProjects(bisque_PROJECTS, *Path))

_bisque_isUser(*Client) =
  if *Client == '' then false
  else
    let *zone = cyverse_ZONE in
    let *ans = false in
    let *_ = foreach (*_ in
        SELECT USER_TYPE WHERE USER_NAME = *Client AND USER_ZONE = *zone AND USER_TYPE = 'rodsuser'
      ) { *ans = true } in
    *ans


_bisque_logMsg(*Msg) {
  writeLine('serverLog', 'BISQUE: *Msg');
}


_bisque_mkIrodsUrl(*Path, *URL) {
  *pathArg = execCmdArg(*Path);
  *status = errorcode(
    msiExecCmd('url-encode-path-segments.sh', *pathArg, cyverse_RE_HOST, "", "", *out) );

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - failed to encode path (*resp)');
    fail;
  } else {
    msiGetStdoutInExecCmdOut(*out, *encodedPath);
    *URL = bisque_IRODS_URL_BASE ++ *encodedPath;
  }
}


# Tells BisQue to create a link for a given user to a data object.
#
# bisque_paths.py (--alias user) ln -P permission /path/to/data.object
#
_bisque_Ln(*Permission, *Client, *Path) {
  if (*Client != '') {
    _bisque_logMsg("linking *Path for *Client with permission *Permission");
  } else {
    _bisque_logMsg("linking *Path with permission *Permission");
  }

  _bisque_mkIrodsUrl(*Path, *irodsUrl);
  *aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
  *pArg = execCmdArg(*Permission);
  *pathArg = execCmdArg(*irodsUrl);
  *argStr = '*aliasOpt ln -P *pArg *pathArg';
  *status = errorcode(
    msiExecCmd("bisque_paths.py", *argStr, cyverse_RE_HOST, "null", "null", *out) );

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    if (*Client != '') {
      _bisque_logMsg('failed to link *Path for *Client with permission *Permission');
	 } else {
      _bisque_logMsg('failed to link *Path with permission *Permission');
    }
    fail;
  } else {
    msiGetStdoutInExecCmdOut(*out, *resp);
    *props = split(trimr(triml(*resp, ' '), '/'), ' ')
    msiStrArray2String(*props, *kvStr);
    msiString2KeyValPair(*kvStr, *kvs);
    msiGetValByKey(*kvs, 'resource_uniq', *qId);
    *id = substr(*qId, 1, strlen(*qId) - 1);
    msiGetValByKey(*kvs, 'uri', *qURI);
    *uri = substr(*qURI, 1, strlen(*qURI) - 1);
    cyverse_setProtectedAVU(*Path, _bisque_ID_ATTR, *id, '');
    cyverse_setProtectedAVU(*Path, _bisque_URI_ATTR, *uri, '');

    if (*Client != '') {
      _bisque_logMsg('linked *Path for *Client with permission *Permission');
	 } else {
      _bisque_logMsg('linked *Path with permission *Permission');
    }
  }
}


# Tells BisQue to change the path of a linked data object.
#
# bisque_paths.py (--alias user) mv /old/path/to/data.object /new/path/to/data.object
#
_bisque_Mv(*Client, *OldPath, *NewPath) {
  if (*Client != '') {
    _bisque_logMsg('moving link from *OldPath to *NewPath for *Client');
  } else {
    _bisque_logMsg('moving link from *OldPath to *NewPath');
  }

  _bisque_mkIrodsUrl(*OldPath, *oldUrl);
  _bisque_mkIrodsUrl(*NewPath, *newUrl);
  *aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
  *oldPathArg = execCmdArg(*oldUrl);
  *newPathArg = execCmdArg(*newUrl);
  *argStr = '*aliasOpt mv *oldPathArg *newPathArg';
  *status = errorcode(
    msiExecCmd('bisque_paths.py', *argStr, cyverse_RE_HOST, 'null', 'null', *out) );

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    if (*Client != '') {
      _bisque_logMsg('failed to move link from *OldPath to *NewPath for *Client');
	 } else {
      _bisque_logMsg('failed to move link from *OldPath to *NewPath');
    }
    fail;
  } else {
    if (*Client != '') {
      _bisque_logMsg('moved link from *OldPath to *NewPath for *Client');
	 } else {
      _bisque_logMsg('moved link from *OldPath to *NewPath');
    }
  }
}


# Tells BisQue to remove a link to data object.
#
# bisque_paths.py (--alias user) rm /path/to/data.object
#
_bisque_Rm(*Client, *Path) {
  if (*Client != '') {
    _bisque_logMsg("removing link from *Path for *Client");
  } else {
    _bisque_logMsg("removing link from *Path");
  }

  _bisque_mkIrodsUrl(*Path, *irodsUrl);
  *aliasOpt = if _bisque_isUser(*Client) then '--alias ' ++ execCmdArg(*Client) else '';
  *pathArg = execCmdArg(*irodsUrl);
  *argStr = '*aliasOpt rm *pathArg';
  *status = errorcode(
    msiExecCmd("bisque_paths.py", *argStr, cyverse_RE_HOST, "null", "null", *out) );

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    if (*Client != '') {
      _bisque_logMsg('failed to remove link to *Path for *Client');
	 } else {
      _bisque_logMsg('failed to remove link to *Path');
    }
    fail;
  } else {
    if (*Client != '') {
      _bisque_logMsg('removed link to *Path for *Client');
	 } else {
      _bisque_logMsg('removed link to *Path');
    }
  }
}


_bisque_scheduleLn(*Permission, *Client, *Path) {
  if (*Client != '') {
    _bisque_logMsg("scheduling linking of *Path for *Client with permission *Permission");
  } else {
    _bisque_logMsg("scheduling linking of *Path with permission *Permission");
  }
# XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See
#       https://github.com/irods/irods/issues/5413.
  #delay("<PLUSET>1s</PLUSET>") {_bisque_Ln(*Permission, *Client, *Path)};
  delay(
    ' <INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
      <PLUSET>1s</PLUSET> '
  ) {_bisque_Ln(*Permission, *Client, *Path)};
}


_bisque_handleNewObject(*Client, *Path) {
  cyverse_giveAccessDataObj(_bisque_USER, 'write', *Path);
  *perm = if _bisque_isInProjects(bisque_PROJECTS, *Path) then 'published' else 'private';
  _bisque_scheduleLn(*perm, *Client, *Path);
}


_bisque_handleObjCreate(*CreatorName, *CreatorZone, *Path) {
  if (_bisque_isForBisque(*CreatorName, *Path)) {
    _bisque_handleNewObject(_bisque_getClient(*CreatorName, *CreatorZone, *Path), *Path);
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

    foreach (*collPat in list(*DestEntity, *DestEntity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collName = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;

        if (_bisque_isInBisque(*collName, *dataName)) {
          *srcSubColl = _bisque_determineSrc(*SrcEntity, *DestEntity, *collName);
          _bisque_Mv(*client, '*srcSubColl/*dataName', '*collName/*dataName');
        } else if (*forBisque) {
          _bisque_handleNewObject(*client, '*collName/*dataName');
        }
      }
    }
  } else if (cyverse_isDataObj(*type)) {
    msiSplitPath(*DestEntity, *collName, *dataName);

    if (_bisque_isInBisque(*collName, *dataName)) {
      _bisque_Mv(*client, *SrcEntity, *DestEntity);
    } else if (*forBisque) {
      _bisque_handleNewObject(*client, *DestEntity);
    }
  }
}


# Add a call to this rule from inside the acDataDeletePolicy PEP.
bisque_acDataDeletePolicy {
  msiSplitPath($objPath, *collName, *dataName);
  temporaryStorage.'bisque_$objPath' = if _bisque_isInBisque(*collName, *dataName) then 'rm' else ''
}


# Add a call to this rule from inside the acPostProcForDelete PEP.
bisque_acPostProcForDelete {
  if (temporaryStorage.'bisque_$objPath' == 'rm') {
    _bisque_Rm(_bisque_getClient($userNameClient, $rodsZoneClient, $objPath), $objPath);
  }
}


bisque_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO) {
  _bisque_handleObjCreate(*User, *Zone, *DATA_OBJ_INFO.logical_path);
}
