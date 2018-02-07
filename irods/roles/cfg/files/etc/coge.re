# VERSION: 2
#
# coge.re
#
# COGE related rules
# put this in server/config/reConfigs/coge.re


_coge_USER = 'coge'
_coge_COLL = 'coge_data'


isForCoge(*Path) = *Path like '/' ++ ipc_ZONE ++ '/home/*/' ++ _coge_COLL ||
                   *Path like '/' ++ ipc_ZONE ++ '/home/*/' ++ _coge_COLL ++ '/*'


setCogePerm(*Path) {
  *lvl = if *Path == '/' ++ ipc_ZONE ++ '/home/' ++ _coge_USER ++ '/' ++ _coge_COLL ||
            *Path like '/' ++ ipc_ZONE ++ '/home/' ++ coge_USER ++ '/' ++ _coge_COLL ++ '/*' ||
            *Path like '/' ++ ipc_ZONE ++ '/home/' ++ coge_USER ++ '/*/' ++ _coge_COLL ||
            *Path like '/' ++ ipc_ZONE ++ '/home/' ++ coge_USER ++ '/*/' ++ _coge_COLL ++ '/*'
         then 'own'
         else 'write';
  msiSetACL('default', *lvl, _coge_USER, *Path);
}


ensureCogeWritePermColl(*Path) {
  setCogePerm(*Path);
  msiSetACL('recursive', 'inherit', 'null', *Path);
}


ensureCogeWritePermObj(*Path) {
  setCogePerm(*Path);
}


coge_acPostProcForCollCreate {
  if(isForCoge($collName)) {
    writeLine('serverLog', 'COGE: permitting ' ++ _coge_USER ++ ' user RW on $collName');
    ensureCogeWritePermColl($collName);
  }
}


coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  if(!isForCoge(*SrcEntity) && isForCoge(*DestEntity)) {
    msiGetObjType(*DestEntity, *type);
    if (*type == '-c') {
      ensureCogeWritePermColl(*DestEntity);

      # Ensure all member collections have the correct permissions
      msiExecStrCondQuery("SELECT COLL_NAME WHERE COLL_NAME LIKE '*DestEntity/%'", *results);
      foreach(*row in *results) {
        msiGetValByKey(*row, "COLL_NAME", *coll);
        ensureCogeWritePermColl(*coll);
      }

      # Ensure all immediate member data objects have the correct permissions
      msiExecStrCondQuery("SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME = '*DestEntity'", *results);
      foreach(*row in *results) {
        msiGetValByKey(*row, "COLL_NAME", *coll);
        msiGetValByKey(*row, "DATA_NAME", *obj);
        ensureCogeWritePermObj(*coll++'/'++*obj);
      }

      # Ensure the data objects more deeply nested have the correct permissions
      msiExecStrCondQuery("SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*DestEntity/%'",
                          *results);
      foreach(*row in *results) {
        msiGetValByKey(*row, "COLL_NAME", *coll);
        msiGetValByKey(*row, "DATA_NAME", *obj);
        ensureCogeWritePermObj(*coll++'/'++*obj);
      }
    } else if (*type == '-d') {
      ensureCogeWritePermObj(*DestEntity);
    }
  }
}
