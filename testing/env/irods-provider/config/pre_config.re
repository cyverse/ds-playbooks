# This is a stub configuration to making testing work

_cyverse_housekeeping_updateQuotaUsage {}
cyverse_housekeeping_rescheduleQuotaUsageUpdate {
	delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>') {_cyverse_housekeeping_updateQuotaUsage};
}

_cyverse_housekeeping_determineAllStorageFreeSpace {}
cyverse_housekeeping_rescheduleStorageFreeSpaceDetermination {
	delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>')
	{_cyverse_housekeeping_determineAllStorageFreeSpace};
}

_cyverse_housekeeping_rmTrash {}
cyverse_housekeeping_rescheduleTrashRemoval {
	delay('<PLUSET>0s</PLUSET><EF>1h REPEAT FOR EVER</EF>') {_cyverse_housekeeping_rmTrash};
}


acCreateUser {
	on ($otherUserType == 'ds-service') {
		msiCreateUser ::: msiRollback;
		msiCommit;
	}
}
