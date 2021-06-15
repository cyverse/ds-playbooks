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


coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_coge_USER, _coge_COLL, _coge_PERM, *SrcEntity, *DestEntity);
}


coge_dataObjCreated(*_, *_, *DATA_OBJ_INFO) {
  ipc_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, _coge_PERM, *DATA_OBJ_INFO.logical_path);
}
