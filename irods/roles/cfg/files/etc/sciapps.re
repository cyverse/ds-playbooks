# VERSION 1
#
# sciapps.re
#
# SciApps related rules

_sciapps_COLL = 'sci_data'
_sciapps_USER = 'maizecode'


sciapps_acPostProcForCollCreate {
  ipc_ensureAccessOnCreateColl(_sciapps_USER, _sciapps_COLL, $collName);
}


sciapps_acPostProcForCopy {
  ipc_ensureAccessOnCreateObj(_sciapps_USER, _sciapps_COLL, $dataPath);
}


sciapps_acPostProcForPut {
  ipc_ensureAccessOnCreateObj(_sciapps_USER, _sciapps_COLL, $dataPath);
}


sciapps_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_sciapps_USER, _sciapps_COLL, *SrcEntity, *DestEntity);
}
