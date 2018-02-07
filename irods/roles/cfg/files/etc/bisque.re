# VERSION 9
#
# bisque.re
# Bisque related rules
# put this in server/config/reConfigs/bisque.re
# include this file from within ipc-custom.re

@include 'bisque-env'

_bisque_COLL = 'bisque_data'
_bisque_ID_ATTR = 'ipc-bisque-id'
_bisque_URI_ATTR = 'ipc-bisque-uri'
_bisque_USER = 'bisque'

_bisque_stripTrailingSlash(*Path) = if *Path like '*/' then trimr(*Path, '/') else *Path

_bisque_determineSrc(*BaseSrcColl, *BaseDestColl, *DestEntity) =
  let *dest = _bisque_stripTrailingSlash(*DestEntity) in
  _bisque_stripTrailingSlash(*BaseSrcColl)
    ++ substr(*dest, strlen(_bisque_stripTrailingSlash(*BaseDestColl)), strlen(*dest))

_bisque_getHomeUser(*Path) =
  let *nodes = split(*Path, '/') in
  if size(*nodes) <= 2
  then ''
  else let *user = elem(*nodes, 2) in
       if *user == 'shared' then '' else *user

_bisque_getClient(*Path) =
  let *homeUser = _bisque_getHomeUser(*Path) in
  if *homeUser == '' then $userNameClient else *homeUser

_bisque_isInBisque(*CollName, *DataName) =
  let *idAttr = _bisque_ID_ATTR in
  foreach (*reg in SELECT COUNT(META_DATA_ATTR_VALUE)
                   WHERE COLL_NAME = '*CollName'
                     AND DATA_NAME = '*DataName'
                     AND META_DATA_ATTR_NAME = '*idAttr') {
    *reg.META_DATA_ATTR_VALUE != '0'
  }

_bisque_isInProject(*Project, *Path) = *Path like '/' ++ ipc_ZONE ++ '/home/shared/*Project/\*'

_bisque_inInProjects(*Projects, *Path) =
  if size(*Projects) == 0
  then false
  else if _bisque_isInProject(hd(*Projects), *Path)
       then true
       else _bisque_inInProjects(tl(*Projects), *Path)

_bisque_isInProjects(*Path) = _bisque_isInProjects(bisque_PROJECTS, *Path)

_bisque_isInUser(*Path) =
  *Path like regex '/' ++ ipc_ZONE ++ '/home/[^/]\*/' ++ _bisque_COLL ++ '($|/.\*)'
  && !(*Path like '/' ++ ipc_ZONE ++ '/home/shared/\*')

_bisque_isForBisque(*Path) =
  $userNameClient != _bisque_USER && (_bisque_isInUser(*Path) || _bisque_isInProjects(*Path))

_bisque_joinPath(*ParentColl, *ObjName) = *ParentColl ++ '/' ++ *ObjName

_bisque_mkIrodsUrl(*Path) = bisque_IRODS_URL_BASE ++ *Path


_bisque_logMsg(*Msg) {
  writeLine('serverLog', 'BISQUE: *Msg');
}


# Tells BisQue to create a link for a given user to a data object.
#
# bisque_ops.py --alias user ln -P permission /path/to/data.object
#
_bisque_Ln() {
  _bisque_logMsg("linking *Path for *Client with permission *Permission");

  *pArg = execCmdArg(*Permission);
  *aliasArg = execCmdArg(*Client);
  *pathArg = execCmdArg(_bisque_mkIrodsUrl(*Path));
  *argStr = '--alias *aliasArg -P *pArg ln *pathArg';
  *status = errorcode(msiExecCmd("bisque_ops.py", *argStr, *IESHost, "null", "null", *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to link *Path for *Client with permission *Permission');
    fail;
  } else {
    # bisque_ops.py exits normally even when an error occurs.

    msiGetStderrInExecCmdOut(*out, *errMsg);

    if (strlen(*errMsg) > 0) {
      _bisque_logMsg(*errMsg);
      _bisque_logMsg('failed to link *Path for *Client with permission *Permission');
      fail;
    }

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
# bisque_ops.py --alias user mv /old/path/to/data.object /new/path/to/data.object
#
_bisque_Mv(*IESHost, *Permission, *Client, *Path) {
  _bisque_logMsg('moving link from *OldPath to *NewPath for *Client');

  *aliasArg = execCmdArg(*Client);
  *oldPathArg = execCmdArg(_bisque_mkIrodsUrl(*OldPath));
  *newPathArg = execCmdArg(_bisque_mkIrodsUrl(*NewPath));
  *argStr = '--alias *aliasArg mv *oldPathArg *newPathArg';
  *status = errorcode(msiExecCmd('bisque_ops.py', *argStr, *IESHost, 'null', 'null', *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to move link from *OldPath to *NewPath for *Client');
    fail;
  } else {
    # bisque_ops.py exits normally even when an error occurs.

    msiGetStderrInExecCmdOut(*out, *errMsg);

    if (strlen(*errMsg) > 0) {
      _bisque_logMsg(*errMsg);
      _bisque_logMsg('failed to move link from *OldPath to *NewPath for *Client');
      fail;
    }

    _bisque_logMsg('moved link from *OldPath to *NewPath for *Client');
  }
}


# Tells BisQue to remove a link to data object.
#
# bisque_ops.py --alias user rm /path/to/data.object
#
_bisque_Rm(*IESHost, *Client, *Path) {
  _bisque_logMsg("Removing link from *Path for *Client");

  *aliasArg = execCmdArg(*Client);
  *pathArg = execCmdArg(_bisque_mkIrodsUrl(*Path));
  *argStr = '--alias *aliasArg rm *pathArg';
  *status = errorcode(msiExecCmd("bisque_ops.py", *argStr, *IESHost, "null", "null", *out));

  if (*status != 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    _bisque_logMsg('FAILURE - *resp');
    _bisque_logMsg('failed to remove link to *Path for *Client');
    fail;
  } else {
    # bisque_ops.py exits normally even when an error occurs.

    msiGetStderrInExecCmdOut(*out, *errMsg);

    if (strlen(*errMsg) > 0) {
      _bisque_logMsg(*errMsg);
      _bisque_logMsg('failed to remove link to *Path for *Client');
      fail;
    }

    _bisque_logMsg('removed link to *Path for *Client');
  }
}


_bisque_scheduleLn(*IESHost, *Permission, *Client, *Path) {
  _bisque_logMsg("scheduling linking of *Path for *Client with permission *Permission");

  delay("<PLUSET>1s</PLUSET>") {
    _bisque_Ln(*IESHost, *Permission, *Client, *Path);
  }
}


_bisque_scheduleMv(*IESHost, *Client, *OldPath, *NewPath) {
  _bisque_logMsg('scheduling link move from *OldPath to *NewPath for *Client');

  delay("<PLUSET>1s</PLUSET>") {
    _bisque_Mv(*IESHost, *Client, *OldPath, *NewPath);
  }
}


_bisque_scheduleRm(*IESHost, *Client, *Path) {
  _bisque_logMsg("scheduling removal of linking to *Path for *Client");

  delay("<PLUSET>1s</PLUSET>") {
    _bisque_Mv(*IESHost, *Client, *Path);
  }
}


_bisque_ensureBisqueWritePerm(*Path) {
  msiSetACL('default', 'write', _bisque_USER, *Path);
}


_bisque_ensureBisqueWritePermColl(*Path) {
  _bisque_logMsg('permitting ' ++ _bisque_USER ++ ' user RW on *Path');
  msiSetACL('recursive', 'write', _bisque_USER, *Path);
}


_bisque_handleNewObject(*IESHost, *Client, *Path) {
  _bisque_ensureBisqueWritePerm(*Path);
  *perm = if _bisque_isInProjects(*Path) then 'published' else 'private';
  _bisque_scheduleLn(*IESHost, *perm, *Client, *Path);
}


# Add a call to this rule from inside the acPostProcForCollCreate PEP.
bisque_acPostProcForCollCreate {
  if (_bisque_isForBisque($collName)) {
    _bisque_ensureBisqueWritePermColl($collName);
  }
}


# Add a call to this rule from inside the acPostProcForPut PEP.
bisque_acPostProcForPut(*IESHost) {
  if (_bisque_isForBisque($objPath)) {
    _bisque_handleNewObject(*IESHost, _bisque_getClient($objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForCopy PEP.
bisque_acPostProcForCopy(*IESHost) {
  if (_bisque_isForBisque($objPath)) {
    _bisque_handleNewObject(*IESHost, _bisque_getClient($objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForObjRename PEP.
bisque_acPostProcForObjRename(*SrcEntity, *DestEntity, *IESHost) {
  *client = _bisque_getClient(*SrcEntity);
  *forBisque = _bisque_isForBisque(*DestEntity);
  msiGetObjType(*DestEntity, *type);

  if (*type == '-c') {
    if (*forBisque) {
      _bisque_ensureBisqueWritePermColl(*DestEntity);
    }

    foreach (*collPat in list(*DestEntity, *DestEntity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collName = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;

        if (_bisque_isInBisque(*collName, *dataName)) {
          *srcSubColl = _bisque_determineSrc(*SrcEntity, *DestEntity, *collName);
          *srcObj = _bisque_joinPath(*srcSubColl, *dataName);
          *destObj = _bisque_joinPath(*collName, *dataName);
          _bisque_scheduleMv(*IESHost, *client, *srcObj, *destObj);
        } else if (*forBisque) {
          _bisque_handleNewObject(*IESHost, *client, _bisque_joinPath(*collName, *dataName));
        }
      }
    }
  } else if (*type == '-d') {
    msiSplitPath(*DestEntity, *collName, *dataName);

    if (_bisque_isInBisque(*collName, *dataName)) {
      _bisque_scheduleMv(*IESHost, *client, *SrcEntity, *DestEntity);
    } else if (*forBisque) {
      _bisque_handleNewObject(*IESHost, *client, *DestEntity);
    }
  }
}


# Add a call to this rule from inside the acPostProcForDelete PEP.
bisque_acPostProcForDelete(*IESHost) {
  if (_bisque_isInUser($objPath) || _bisque_isInProjects($objPath)) {
    _bisque_scheduleRm(*IESHost, _bisque_getClient($objPath), $objPath);
  }
}
