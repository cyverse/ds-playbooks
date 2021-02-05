# This is just a stub to making testing work

_ipc_updateQuotaUsage {}
ipc_rescheduleQuotaUsageUpdate {
  delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>') {_ipc_updateQuotaUsage};
}

ipc_rescheduleStorageFreeSpaceDetermination {}
ipc_rescheduleTrashRemoval {}
