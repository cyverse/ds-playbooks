# VERSION: 2
#
# coge.re
#
# COGE related rules
# put this in server/config/reConfigs/coge.re


_coge_USER = 'coge'
_coge_COLL = 'coge_data'


coge_acPostProcForCollCreate {
  ipc_ensureAccessOnCreateColl(_coge_USER, _coge_COLL, $collName);
}


coge_acPostProcForCopy {
  ipc_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, $dataPath);
}


coge_acPostProcForPut {
  ipc_ensureAccessOnCreateObj(_coge_USER, _coge_COLL, $dataPath);
}


coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_coge_USER, _coge_COLL, *SrcEntity, *DestEntity);
}
