# Trash management policy

# generates a unique variable name for a data object or collection based on its absolute path,
# the variable name is prefixed with "trash_timestamp_".
#
# Parameters:
#  *Path:  the absolute path to the data object or collection
#
# Return:
#  the variable name to be used in temporaryStorage to store a timestamp value
#
_ipcTrash_mkTimestampVar: path -> string
_ipcTrash_mkTimestampVar(*Path) = 'trash_timestamp_' ++ str(*Path)

# generates a unique variable name for a data object based on its absolute path,
# the variable name is prefixed with "data_id_".
#
# Parameters:
#  *Path:  the absolute path to the data object
#
# Return:
#  the variable name to be used in temporaryStorage to store a DATA_ID
#
_ipcTrash_mkObjDataIdVar: path -> string
_ipcTrash_mkObjDataIdVar(*Path) = 'data_id_' ++ str(*Path)

_ipcTrash_manageTimeAVU(*action, *type, *path, *avuValue) {
  *actionArg = execCmdArg(*action);
  *typeArg = execCmdArg(*type);
  *pathArg = execCmdArg(*path);
  *avuValueArg = execCmdArg(*avuValue);
  *avuName = execCmdArg("ipc::trash_timestamp");
  *argv = "*actionArg *typeArg *pathArg *avuName *avuValueArg";
  *err = errormsg(msiExecCmd('imeta-exec', *argv, "", "", "", *out), *msg);
  if (*err < 0) {
    msiGetStderrInExecCmdOut(*out, *resp);
    writeLine('serverLog', 'imeta-exec stderr: *resp');
    writeLine('serverLog', '_ipcTrash_manageTimeAVU: *msg');
    *err;
  }
}

ipcTrash_api_data_obj_unlink_pre(*INSTANCE, *COMM, *DATAOBJUNLINKINP) {
  if (errorcode(*DATAOBJUNLINKINP.forceFlag) != 0) {
    msiGetSystemTime(*timestamp, "");
    *dataObjPath = *DATAOBJUNLINKINP.obj_path;
    *timestampVar = _ipcTrash_mkTimestampVar(/*dataObjPath);
    temporaryStorage.'*timestampVar' = *timestamp;
    _ipcTrash_manageTimeAVU("set", ipc_DATA_OBJECT, *dataObjPath, *timestamp);

    msiSplitPath(*dataObjPath, *coll, *file);
    foreach(*row in SELECT DATA_ID WHERE COLL_NAME = '*coll' AND DATA_NAME = '*file') {
      *dataIdVar = _ipcTrash_mkObjDataIdVar(/*dataObjPath);
      temporaryStorage.'*dataIdVar' = *row.DATA_ID;
    }
  }
}

ipcTrash_api_data_obj_unlink_post(*INSTANCE, *COMM, *DATAOBJUNLINKINP) {
  *dataObjPath = *DATAOBJUNLINKINP.obj_path;
  *timestampVar = _ipcTrash_mkTimestampVar(/*dataObjPath);
  if (errorcode(temporaryStorage.'*timestampVar') == 0) {
    temporaryStorage.'*timestampVar' = "";
  }
  *dataIdVar = _ipcTrash_mkObjDataIdVar(/*dataObjPath);
  if (errorcode(temporaryStorage.'*dataIdVar') == 0) {
    *dataIdVarTemp = temporaryStorage.'*dataIdVar';
    foreach(*Row in SELECT COLL_NAME WHERE DATA_ID = '*dataIdVarTemp') {
      *collNameList = split(*Row.COLL_NAME, '/');
      if (size(*collNameList) >= 5) {
        *parentCollPath = "";
        for (*i = 0; *i < 5; *i = *i + 1) {
          *parentCollPath = *parentCollPath ++ "/" ++ elem(*collNameList, *i);
        }
        msiGetSystemTime(*timestamp, "");
        _ipcTrash_manageTimeAVU("set", ipc_COLLECTION, *parentCollPath, *timestamp);
      }
    }
  }
}

ipcTrash_api_data_obj_unlink_except(*INSTANCE, *COMM, *DATAOBJUNLINKINP) {
  *dataObjPath = *DATAOBJUNLINKINP.obj_path;
  *timestampVar = _ipcTrash_mkTimestampVar(/*dataObjPath);
  if (errorcode(temporaryStorage.'*timestampVar') == 0) {
    if (temporaryStorage.'*timestampVar' != "") {
      _ipcTrash_manageTimeAVU(
        "rm", ipc_DATA_OBJECT, *dataObjPath, temporaryStorage.'*timestampVar' );
    }
  }
}

ipcTrash_api_data_obj_put_post(*INSTANCE, *COMM, *DATAOBJINP, *DATAOBJINPBBUF, *PORTALOPROUT) {
  *zone = cyverse_ZONE;
  if (*DATAOBJINP.obj_path like '/*zone/trash/*') {
    msiGetSystemTime(*timestamp, "");
    _ipcTrash_manageTimeAVU("set", ipc_DATA_OBJECT, *DATAOBJINP.obj_path, *timestamp);
  }
}

ipcTrash_api_rm_coll_pre(*INSTANCE, *COMM, *RMCOLLINP, *COLLOPRSTAT) {
  if (errorcode(*RMCOLLINP.forceFlag) != 0) {
    msiGetSystemTime(*timestamp, "");
    *collNamePath = *RMCOLLINP.coll_name;
    *timestampVar = _ipcTrash_mkTimestampVar(/*collNamePath);
    temporaryStorage.'*timestampVar' = *timestamp;
    _ipcTrash_manageTimeAVU("set", ipc_COLLECTION, *collNamePath, *timestamp);
  }
}

ipcTrash_api_rm_coll_except(*INSTANCE, *COMM, *RMCOLLINP, *COLLOPRSTAT) {
  *collNamePath = *RMCOLLINP.coll_name;
  *timestampVar = _ipcTrash_mkTimestampVar(/*collNamePath);
  if (errorcode(temporaryStorage.'*timestampVar') == 0) {
    _ipcTrash_manageTimeAVU("rm", ipc_COLLECTION, *collNamePath, temporaryStorage.'*timestampVar');
  }
}

ipcTrash_api_coll_create_post(*INSTANCE, *COMM, *COLLCREATEINP) {
  *zone = cyverse_ZONE;
  *collNamePath = *COLLCREATEINP.coll_name;
  if (*collNamePath like '/*zone/trash/*') {
    msiGetSystemTime(*timestamp, "");
    _ipcTrash_manageTimeAVU("set", ipc_COLLECTION, *collNamePath, *timestamp);
  }
}

ipcTrash_api_data_obj_rename_pre(*INSTANCE, *COMM, *DATAOBJRENAMEINP) {
  *zone = cyverse_ZONE;
  if (
    (*DATAOBJRENAMEINP.src_obj_path like '/*zone/trash/*')
    && (*DATAOBJRENAMEINP.dst_obj_path not like '/*zone/trash/*')
  ) {
    *srcObjPath = *DATAOBJRENAMEINP.src_obj_path;
    *timestampVar = _ipcTrash_mkTimestampVar(/*srcObjPath);
    msiGetObjType(*srcObjPath, *Type);
    if (ipc_isCollection(*Type)) {
      foreach(*Row in SELECT META_COLL_ATTR_VALUE
                        WHERE COLL_NAME like '*srcObjPath'
                          AND META_COLL_ATTR_NAME = 'ipc::trash_timestamp') {
                            temporaryStorage.'*timestampVar' = *Row.META_COLL_ATTR_VALUE;
      }
    }
    else if (ipc_isDataObject(*Type)) {
      msiSplitPath(*srcObjPath, *Coll, *File);
      foreach(*Row in SELECT META_DATA_ATTR_VALUE
                        WHERE COLL_NAME like '*Coll'
                          AND DATA_NAME like '*File'
                            AND META_DATA_ATTR_NAME = 'ipc::trash_timestamp') {
                              temporaryStorage.'*timestampVar' = *Row.META_DATA_ATTR_VALUE;
      }
    }
  }
}

ipcTrash_api_data_obj_rename_post(*INSTANCE, *COMM, *DATAOBJRENAMEINP) {
  *zone = cyverse_ZONE;
  *destObjPath = *DATAOBJRENAMEINP.dst_obj_path;
  if (*destObjPath like '/*zone/trash/*') {
    msiGetSystemTime(*timestamp, "");
    _ipcTrash_manageTimeAVU("set", ipc_getEntityType(*destObjPath), *destObjPath, *timestamp);
  }
  else if (
    (*DATAOBJRENAMEINP.src_obj_path like '/*zone/trash/*')
    && (*DATAOBJRENAMEINP.dst_obj_path not like '/*zone/trash/*')
  ) {
    *srcObjPath = *DATAOBJRENAMEINP.src_obj_path;
    *timestampVar = _ipcTrash_mkTimestampVar(/*srcObjPath);
    if (errorcode(temporaryStorage.'*timestampVar') == 0) {
      _ipcTrash_manageTimeAVU(
        "rm", ipc_getEntityType(*destObjPath), *destObjPath, temporaryStorage.'*timestampVar' );
    }
  }
}

ipcTrash_api_data_obj_copy_post(*INSTANCE, *COMM, *DATAOBJCOPYINP, *TRANSSTAT) {
  *zone = cyverse_ZONE;
  *destObjPath = *DATAOBJCOPYINP.dst_obj_path;
  if (*destObjPath like '/*zone/trash/*') {
    msiGetSystemTime(*timestamp, "");
    _ipcTrash_manageTimeAVU("set", ipc_DATA_OBJECT, *destObjPath, *timestamp);
  }
}

ipcTrash_api_data_obj_create_post(*INSTANCE, *COMM, *DATAOBJINP) {
  *zone = cyverse_ZONE;
  *objPath = *DATAOBJINP.obj_path;
  if (*objPath like '/*zone/trash/*') {
    msiGetSystemTime(*timestamp, "");
    _ipcTrash_manageTimeAVU("set", ipc_DATA_OBJECT, *objPath, *timestamp);
  }
}
