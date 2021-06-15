# VERSION 2
#
# sciapps.re
#
# SciApps related rules

_sciapps_COLL = 'sci_data'
# XXX - Due to
#       https://app.intercom.io/a/apps/tpwq3d9w/inbox/inbox/conversation/15307902609,
#       the required permission is own. Please revert this when the issue has
#       been resolved.
#_sciapps_PERM = 'write'
_sciapps_PERM = 'own'
# XXX - ^^^
_sciapps_USER = 'maizecode'


sciapps_acPostProcForCollCreate {
  ipc_ensureAccessOnCreateColl(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, $collName);
}


sciapps_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  ipc_ensureAccessOnMv(_sciapps_USER, _sciapps_COLL, _sciapps_PERM, *SrcEntity, *DestEntity);
}


sciapps_dataObjCreated(*_, *_, *DATA_OBJ_INFO) {
  ipc_ensureAccessOnCreateObj(
    _sciapps_USER, _sciapps_COLL, _sciapps_PERM, *DATA_OBJ_INFO.logical_path);
}
