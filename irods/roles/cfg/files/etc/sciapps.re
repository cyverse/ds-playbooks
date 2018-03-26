# VERSION 2
#
# sciapps.re
#
# SciApps related rules

_sciapps_COLL = 'sci_data'
_sciapps_PERM = 'write'
_sciapps_USER = 'maizecode'


sciapps_acPostProcForCollCreate {
  ipc_ensureAccessOnCreateColl(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, $collName);
}


sciapps_acPostProcForCopy {
  ipc_ensureAccessOnCreateObj(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, $objPath);
}


sciapps_acPostProcForPut {
  ipc_ensureAccessOnCreateObj(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, $objPath);
}


sciapps_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, *SrcEntity, *DestEntity);
}
