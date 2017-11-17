# VERSION 6
#
# bisque.re  
# Bisque related rules for > iRods 4.0
# put this in server/config/reConfigs/bisque.re
# include this file from within ipc-custom.re
###############################################

@include 'bisque-env'


BISQUE_GROUPS = list('NEVP', 'sernec')
BISQUE_ID_ATTR = 'ipc-bisque-id'
BISQUE_URI_ATTR = 'ipc-bisque-uri'


logMsg(*Msg) = writeLine('serverLog', 'BISQUE: *Msg')


mkIrodsUrl(*Path) = bisque_IRODS_URL_BASE ++ *Path


# Tells BisQue to create a link for a given user to a data object.
#
# bisque_ops.py --alias user ln -P permission /path/to/data.object
ln(*IESHost, *Permission, *Client, *Path) {
  logMsg("scheduling linking of *Path for *Client with permission *Permission");
  delay("<PLUSET>1s</PLUSET>") {
    logMsg("linking *Path for *Client with permission *Permission");
    *pArg = execCmdArg(*Permission);
    *aliasArg = execCmdArg(*Client);
    *pathArg = execCmdArg(mkIrodsUrl(*Path));
    *argStr = '--alias *aliasArg -P *pArg ln *pathArg';
    *status = errorcode(msiExecCmd("bisque_ops.py", *argStr, *IESHost, "null", "null", *out));
    if (*status != 0) {
      msiGetStderrInExecCmdOut(*out, *resp);
      logMsg('FAILURE - *resp');
      logMsg('failed to link *Path for *Client with permission *Permission');
      fail;
    } else {
      # bisque_ops.py exits normally even when an error occurs.
      msiGetStderrInExecCmdOut(*out, *errMsg);
      if (strlen(*errMsg) > 0) {
        logMsg(*errMsg);
        logMsg('failed to link *Path for *Client with permission *Permission');
        fail;
      }
      msiGetStdoutInExecCmdOut(*out, *resp);
      *props = split(trimr(triml(*resp, ' '), '/'), ' ')

# This is broken. See https://github.com/irods/irods/issues/3355
#      msiStrArray2String(*props, *kvStr);
# Workaround
      *kvStr = "";
      foreach(*prop in *props) {
        if (*kvStr == "" ) {
          *kvStr = *prop;
        } else {
          *kvStr = *kvStr ++ "%" ++ *prop;
        }
      }
# End of workaround
      msiString2KeyValPair(*kvStr, *kvs);
      msiGetValByKey(*kvs, 'resource_uniq', *qId);
      *id = substr(*qId, 1, strlen(*qId) - 1);
      msiGetValByKey(*kvs, 'uri', *qURI);
      *uri = substr(*qURI, 1, strlen(*qURI) - 1);
      msiString2KeyValPair(BISQUE_ID_ATTR ++ '=' ++ *id ++ '%' ++ BISQUE_URI_ATTR ++ '=' ++ *uri,
                           *kv);
      msiSetKeyValuePairsToObj(*kv, *Path, '-d');
      logMsg('linked *Path for *Client with permission *Permission');
    }
  }
}


# Tells BisQue to change the path of a linked data object.
#
# bisque_ops.py --alias user mv /old/path/to/data.object /new/path/to/data.object
mv(*IESHost, *Client, *OldPath, *NewPath) {
  logMsg('scheduling link move from *OldPath to *NewPath for *Client');
  delay("<PLUSET>1s</PLUSET>") {
    logMsg('moving link from *OldPath to *NewPath for *Client');
    *aliasArg = execCmdArg(*Client);
    *oldPathArg = execCmdArg(mkIrodsUrl(*OldPath));
    *newPathArg = execCmdArg(mkIrodsUrl(*NewPath));
    *argStr = '--alias *aliasArg mv *oldPathArg *newPathArg';
    *status = errorcode(msiExecCmd('bisque_ops.py', *argStr, *IESHost, 'null', 'null', *out));
    if (*status != 0) {
      msiGetStderrInExecCmdOut(*out, *resp);
      logMsg('FAILURE - *resp');
      logMsg('failed to move link from *OldPath to *NewPath for *Client');
      fail;
    } else {
      # bisque_ops.py exits normally even when an error occurs.
      msiGetStderrInExecCmdOut(*out, *errMsg);
      if (strlen(*errMsg) > 0) {
        logMsg(*errMsg);
        logMsg('failed to move link from *OldPath to *NewPath for *Client');
        fail;
      }
      logMsg('moved link from *OldPath to *NewPath for *Client');
    }
  }
}


# Tells BisQue to remove a link to data object.
#
# bisque_ops.py --alias user rm /path/to/data.object
rm(*IESHost, *Client, *Path) {
  logMsg("scheduling removal of linking to *Path for *Client");
  delay("<PLUSET>1s</PLUSET>") {
    logMsg("Removing link from *Path for *Client");
    *aliasArg = execCmdArg(*Client);
    *pathArg = execCmdArg(mkIrodsUrl(*Path));
    *argStr = '--alias *aliasArg rm *pathArg';
    *status = errorcode(msiExecCmd("bisque_ops.py", *argStr, *IESHost, "null", "null", *out));
    if (*status != 0) {
      msiGetStderrInExecCmdOut(*out, *resp);
      logMsg('FAILURE - *resp');
      logMsg('failed to remove link to *Path for *Client');
      fail;
    } else {
      # bisque_ops.py exits normally even when an error occurs.
      msiGetStderrInExecCmdOut(*out, *errMsg);
      if (strlen(*errMsg) > 0) {
        logMsg(*errMsg);
        logMsg('failed to remove link to *Path for *Client');
        fail;
      }
      logMsg('removed link to *Path for *Client');
    }
  }
}


joinPath(*ParentColl, *ObjName) = *ParentColl ++ '/' ++ *ObjName


getHomeUser(*Path) =
  let *nodes = split(*Path, '/') 
  in if size(*nodes) <= 2
     then '' 
     else let *user = elem(*nodes, 2)     
          in if *user == 'shared' then '' else *user


getClient(*Path) =
   let *homeUser = getHomeUser(*Path)
   in if *homeUser == '' then $userNameClient else *homeUser


ensureBisqueWritePerm(*Path) = msiSetACL('default', 'write', 'bisque', *Path)


ensureBisqueWritePermColl(*Path) {
  logMsg('permitting bisque user RW on *Path');
  msiSetACL('recursive', 'write', 'bisque', *Path);
}


isInGroup(*Group, *Path) = *Path like '/iplant/home/shared/*Group/\*'


isInGroups(*Groups, *Path) {
  *result = false;
  foreach(*group in *Groups) {
    if (isInGroup(*group, *Path)) {
      *result = true;
      break;
    }
  }
  *result;
}


isInUser(*Path) = *Path like regex '/iplant/home/[^/]\*/bisque_data($|/.\*)' 
                  && !(*Path like '/iplant/home/shared/\*')


isForBisque(*Path) = $userNameClient != "bisque" 
                     && (isInUser(*Path) || isInGroups(BISQUE_GROUPS, *Path))


handleNewObject(*IESHost, *Client, *Path) {
  ensureBisqueWritePerm(*Path);
  *perm = if isInGroups(BISQUE_GROUPS, *Path) then 'published' else 'private';
  ln(*IESHost, *perm, *Client, *Path);
}


stripTrailingSlash(*Path) = if *Path like '*/' then trimr(*Path, '/') else *Path


determineSrc(*BaseSrcColl, *BaseDestColl, *DestEntity) = 
  let *dest = stripTrailingSlash(*DestEntity) in
    stripTrailingSlash(*BaseSrcColl) ++ 
      substr(*dest, strlen(stripTrailingSlash(*BaseDestColl)), strlen(*dest))


isInBisque(*CollName, *DataName) {
  *inBisque = false;
  *idAttr = BISQUE_ID_ATTR;

  foreach (*reg in SELECT COUNT(META_DATA_ATTR_VALUE)
                   WHERE COLL_NAME = '*CollName' 
                     AND DATA_NAME = '*DataName' 
                     AND META_DATA_ATTR_NAME = '*idAttr') {
    *inBisque = *reg.META_DATA_ATTR_VALUE != '0';
  }

  *inBisque;
}


# Add a call to this rule from inside the acPostProcForCollCreate PEP.
bisque_acPostProcForCollCreate {
  if (isForBisque($collName)) {
    ensureBisqueWritePermColl($collName);
  }   
}


# Add a call to this rule from inside the acPostProcForPut PEP. 
bisque_acPostProcForPut(*IESHost) {
  if (isForBisque($objPath)) {
    handleNewObject(*IESHost, getClient($objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForCopy PEP.
bisque_acPostProcForCopy(*IESHost) {
  if (isForBisque($objPath)) {
    handleNewObject(*IESHost, getClient($objPath), $objPath);
  }
}


# Add a call to this rule from inside the acPostProcForObjRename PEP.
bisque_acPostProcForObjRename(*SrcEntity, *DestEntity, *IESHost) {
  *client = getClient(*SrcEntity);
  *forBisque = isForBisque(*DestEntity);
  msiGetObjType(*DestEntity, *type);
 
  if (*type == '-c') {
    if (*forBisque) {
      ensureBisqueWritePermColl(*DestEntity);
    }

    foreach (*collPat in list(*DestEntity, *DestEntity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collName = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;

        if (isInBisque(*collName, *dataName)) {
          *srcSubColl = determineSrc(*SrcEntity, *DestEntity, *collName);
          *srcObj = joinPath(*srcSubColl, *dataName);
          *destObj = joinPath(*collName, *dataName);
          mv(*IESHost, *client, *srcObj, *destObj);
        } else if (*forBisque) {
          handleNewObject(*IESHost, *client, joinPath(*collName, *dataName));
        }   
      }   
    }
  } else if (*type == '-d') {
    msiSplitPath(*DestEntity, *collName, *dataName);

    if (isInBisque(*collName, *dataName)) {
      mv(*IESHost, *client, *SrcEntity, *DestEntity);
    } else if (*forBisque) {
      handleNewObject(*IESHost, *client, *DestEntity);
    }
  }
}


# Add a call to this rule from inside the acPostProcForDelete PEP.
bisque_acPostProcForDelete(*IESHost) {
  if (isInUser($objPath) || isInGroups(BISQUE_GROUPS, $objPath)) {
    rm(*IESHost, getClient($objPath), $objPath);
  }
}

