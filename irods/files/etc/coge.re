# VERSION: 3
#
# coge.re
#
# CoGe related rules

_coge_COLL = 'coge_data'
_coge_PERM = 'write'
_coge_USER = 'coge'


coge_acPostProcForCollCreate {
  ipc_ensureAccessOnCreateColl(_coge_USER, _coge_COLL, _coge_PERM, $collName);
}


coge_acPostProcForCopy {
  ipc_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, _coge_PERM, $objPath);
}


coge_acPostProcForPut {
  ipc_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, _coge_PERM, $objPath);
}


coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_coge_USER, _coge_COLL, _coge_PERM, *SrcEntity, *DestEntity);
}
