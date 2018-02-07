# VERSION: 2
#
# coge.re
#
# COGE related rules
# put this in server/config/reConfigs/coge.re


_coge_USER = 'coge'
_coge_COLL = 'coge_data'
_coge_HOME = / ++ ipc_ZONE ++ /home


_coge_giveWriteAccessColl(*SvcUser, *CollPath) {
  writeLine('serverLog', 'permitting *SvcUser write access to *CollPath and everything in it');
  msiSetACL('recursive', 'write', *SvcUser, *CollPath);
}


_coge_giveWriteAccessObj(*SvcUser, *ObjPath) {
  writeLine('serverLog', 'permitting *SvcUser write access to *ObjPath');
  msiSetACL('default', 'write', *SvcUser, *ObjPath);
}


_coge_isForService(*SvcUser, *SvcColl, *Path) =
  *Path like regex _ipc_HOME ++ '/[^/]\*/*SvcColl($|/.\*)'
  && !(*Path like _ipc_HOME ++ '/*SvcUser/\*')
  && !(*Path like _ipc_HOME ++ '/shared/\*')


_coge_ensureAccessOnCreateColl(*SvcUser, *SvcColl, *CollPath) {
  if (_coge_isForService(*SvcUser, *SvcColl, *CollPath)) {
    _coge_giveWriteAccessColl(*SvcUser, *CollPath);
  }
}


_coge_ensureAccessOnCreateObj(*SvcUser, *SvcColl, *ObjPath) {
  if (_coge_isForService(*SvcUser, *SvcColl, *ObjPath)) {
    _coge_giveWriteAccessObj(*SvcUser, *ObjPath);
  }
}


_coge_ensureAccessOnMv(*SvcUser, *SvcColl, *OldPath, *NewPath) {
  if (!_coge_isForService(*SvcUser, *SvcColl, *OldPath)
      && _coge_isForService(*SvcUser, *SvcColl, *NewPath)) {

    msiGetObjType(*Path, *type);

    if (*type == '-c') {
      _coge_giveWriteAccessColl(*SvcUser, *Path);
    } else if (*type == '-d') {
      _coge_giveWriteAccessObj(*SvcUser, *Path);
    }
  }
}


coge_acPostProcForCollCreate {
  _coge_ensureAccessOnCreateColl(_coge_USER, _coge_COLL, $collName);
}


coge_acPostProcForCopy {
  _coge_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, $dataPath);
}


coge_acPostProcForPut {
  _coge__ensureAccessOnCreateObj(_coge_USER, _coge_COLL, $dataPath);
}


coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  _coge_ensureAccessOnMv(_coge_USER, _coge_COLL, *SrcEntity, *DestEntity);
}
