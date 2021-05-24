# This is a stub configuration to making testing work

_ipc_updateQuotaUsage {}
ipc_rescheduleQuotaUsageUpdate {
  delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>') {_ipc_updateQuotaUsage};
}

ipc_rescheduleStorageFreeSpaceDetermination {}
ipc_rescheduleTrashRemoval {}


acCreateUser {
  on ($otherUserType == 'ds-service') {
    msiCreateUser ::: msiRollback;
    msiCommit;
  }
}
