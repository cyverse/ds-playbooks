# VERSION 10
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
  if *Path like regex '^/iplant/home/shared($|/.*)' then ''
  else if *Path like regex '^/iplant/home/[^/]+/.+' then elem(split(*Path, '/'), 2)
  else if *Path like regex '/iplant/trash/home/[^/]+/.+' then elem(split(*Path, '/'), 3)
  else ''

_bisque_getClient(*Author, *Path) =
  let *homeUser = _bisque_getHomeUser(*Path) in if *homeUser == '' then *Author else *homeUser

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

_bisque_isInProject(*Project, *Path) = *Path like '/' ++ ipc_ZONE ++ '/home/shared/*Project/*'

_bisque_isInProjects(*Projects, *Path) =
  if size(*Projects) == 0
  then false
  else if _bisque_isInProject(hd(*Projects), *Path)
       then true
       else _bisque_isInProjects(tl(*Projects), *Path)

_bisque_isForBisque(*Author, *Path) =
  *Author != _bisque_USER
  && (ipc_isForService(_bisque_USER, _bisque_COLL, /*Path)
      || _bisque_isInProjects(bisque_PROJECTS, *Path))

_bisque_mkIrodsUrl(*Path) = bisque_IRODS_URL_BASE ++ *Path

_bisque_logMsg(*Msg) {
  writeLine('serverLog', 'BISQUE: *Msg');
}


# Tells BisQue to create a link for a given user to a data object.
#
# bisque_paths.py --alias user ln -P permission /path/to/data.object
#
_bisque_Ln(*Permission, *Client, *Path) {
  _bisque_logMsg("linking *Path for *Client with permission *Permission");

  *pArg = execCmdArg(*Permission);
  *aliasArg = execCmdArg(*Client);
  *pathArg = execCmdArg(_bisque_mkIrodsUrl(*Path));
  *argStr = '--alias *aliasArg -P *pArg ln *pathArg';
  *status = errorcode(msiExecCmd("bisque_paths.py", *argStr, ipc_RE_HOST, "null", "null", *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to link *Path for *Client with permission *Permission');
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

    msiString2KeyValPair(_bisque_ID_ATTR ++ '=' ++ *id ++ '%' ++ _bisque_URI_ATTR ++ '=' ++ *uri,
                         *kv);

    msiSetKeyValuePairsToObj(*kv, *Path, '-d');

    _bisque_logMsg('linked *Path for *Client with permission *Permission');
  }
}


# Tells BisQue to change the path of a linked data object.
#
# bisque_paths.py --alias user mv /old/path/to/data.object /new/path/to/data.object
#
_bisque_Mv(*Client, *OldPath, *NewPath) {
  _bisque_logMsg('moving link from *OldPath to *NewPath for *Client');

  *aliasArg = execCmdArg(*Client);
  *oldPathArg = execCmdArg(_bisque_mkIrodsUrl(*OldPath));
  *newPathArg = execCmdArg(_bisque_mkIrodsUrl(*NewPath));
  *argStr = '--alias *aliasArg mv *oldPathArg *newPathArg';
  *status = errorcode(msiExecCmd('bisque_paths.py', *argStr, ipc_RE_HOST, 'null', 'null', *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to move link from *OldPath to *NewPath for *Client');
    fail;
  } else {
    _bisque_logMsg('moved link from *OldPath to *NewPath for *Client');
  }
}


# Tells BisQue to remove a link to data object.
#
# bisque_paths.py --alias user rm /path/to/data.object
#
_bisque_Rm(*Client, *Path) {
  _bisque_logMsg("Removing link from *Path for *Client");

  *aliasArg = execCmdArg(*Client);
  *pathArg = execCmdArg(_bisque_mkIrodsUrl(*Path));
  *argStr = '--alias *aliasArg rm *pathArg';
  *status = errorcode(msiExecCmd("bisque_paths.py", *argStr, ipc_RE_HOST, "null", "null", *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to remove link to *Path for *Client');
    fail;
  } else {
    _bisque_logMsg('removed link to *Path for *Client');
  }
}


_bisque_scheduleLn(*Permission, *Client, *Path) {
  _bisque_logMsg("scheduling linking of *Path for *Client with permission *Permission");

  delay("<PLUSET>1s</PLUSET>") {
# XXX - Due to https://github.com/irods/irods/issues/3621, _bisque_Ln has been inlined. Undo this
#       the next iRODS upgrade.
#    _bisque_Ln(*Permission, *Client, *Path);
    _bisque_logMsg("linking *Path for *Client with permission *Permission");

    *pArg = execCmdArg(*Permission);
    *aliasArg = execCmdArg(*Client);
    *pathArg = execCmdArg(_bisque_mkIrodsUrl(*Path));
    *argStr = '--alias *aliasArg -P *pArg ln *pathArg';
    *status = errorcode(msiExecCmd("bisque_paths.py", *argStr, ipc_RE_HOST, "null", "null", *out));

    if (*status != 0) {
      msiGetStderrInExecCmdOut(*out, *resp);
      _bisque_logMsg('FAILURE - *resp');
      _bisque_logMsg('failed to link *Path for *Client with permission *Permission');
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

      msiString2KeyValPair(_bisque_ID_ATTR ++ '=' ++ *id ++ '%' ++ _bisque_URI_ATTR ++ '=' ++ *uri,
                           *kv);

      msiSetKeyValuePairsToObj(*kv, *Path, '-d');

      _bisque_logMsg('linked *Path for *Client with permission *Permission');
    }
# XXX - ^^^
  }
}


_bisque_scheduleMv(*Client, *OldPath, *NewPath) {
  _bisque_logMsg('scheduling link move from *OldPath to *NewPath for *Client');

  delay("<PLUSET>1s</PLUSET>") {
    _bisque_Mv(*Client, *OldPath, *NewPath);
  }
}


_bisque_scheduleRm(*Client, *Path) {
  _bisque_logMsg("scheduling removal of linking to *Path for *Client");

  delay("<PLUSET>1s</PLUSET>") {
    _bisque_Rm(*Client, *Path);
  }
}


_bisque_handleNewObject(*Client, *Path) {
  ipc_giveAccessObj(_bisque_USER, 'write', *Path);
  *perm = if _bisque_isInProjects(bisque_PROJECTS, *Path) then 'published' else 'private';
  _bisque_scheduleLn(*perm, *Client, *Path);
}


# Add a call to this rule from inside the acPostProcForCollCreate PEP.
bisque_acPostProcForCollCreate {
  if (_bisque_isForBisque($userNameClient, $collName)) {
    ipc_giveAccessColl(_bisque_USER, 'write', $collName);
  }
}


# Add a call to this rule from inside the acPostProcForPut PEP.
bisque_acPostProcForPut {
  if (_bisque_isForBisque($userNameClient, $objPath)) {
    _bisque_handleNewObject(_bisque_getClient($userNameClient, $objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForCopy PEP.
bisque_acPostProcForCopy {
  if (_bisque_isForBisque($userNameClient, $objPath)) {
    _bisque_handleNewObject(_bisque_getClient($userNameClient, $objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForObjRename PEP.
bisque_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  *client = _bisque_getClient($userNameClient, *SrcEntity);
  *forBisque = _bisque_isForBisque($userNameClient, *DestEntity);
  msiGetObjType(*DestEntity, *type);

  if (*type == '-c') {
    if (*forBisque) {
      ipc_giveAccessColl(_bisque_USER, 'write', *DestEntity);
    }

    foreach (*collPat in list(*DestEntity, *DestEntity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collName = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;

        if (_bisque_isInBisque(*collName, *dataName)) {
          *srcSubColl = _bisque_determineSrc(*SrcEntity, *DestEntity, *collName);
          _bisque_scheduleMv(*client, '*srcSubColl/*dataName', '*collName/*dataName');
        } else if (*forBisque) {
          _bisque_handleNewObject(*client, '*collName/*dataName');
        }
      }
    }
  } else if (*type == '-d') {
    msiSplitPath(*DestEntity, *collName, *dataName);

    if (_bisque_isInBisque(*collName, *dataName)) {
      _bisque_scheduleMv(*client, *SrcEntity, *DestEntity);
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
    _bisque_scheduleRm(_bisque_getClient($userNameClient, $objPath), $objPath);
  }
}
