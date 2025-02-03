# CoGe service policy
#
# © 2025 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

_coge_COLL = 'coge_data'
_coge_PERM = 'write'
_coge_USER = 'coge'


### STATIC PEPS

# This rule ensures the CoGe user has write access to a collection created in a
# CoGe data collection.
#
# Session Variables:
#  collName
#
coge_acPostProcForCollCreate {
	cyverse_ensureAccessOnCreateColl(_coge_USER, _coge_COLL, _coge_PERM, $collName);
}

# This rule ensures the CoGe user has write access to anything moved into a CoGe
# data collection.
#
# Parameters:
#  SrcEntity   (path) the path to the collection or object before it was moved
#  DestEntity  (path) the path after the move
#
coge_acPostProcForObjRename(*SrcEntity, *DestEntity) {
	cyverse_ensureAccessOnMv(_coge_USER, _coge_COLL, _coge_PERM, *SrcEntity, *DestEntity);
}


### DYNAMIC PEPS

# This rule ensures the CoGe user has write access to a data object created in a
# CoGe data collection.
#
# Parameters:
#  User         (string) unused
#  Zone         (string) unused
#  DataObjInfo  (`KeyValuePair_PI`) the DATA_OBJ_INFO map for the event creating
#               the data object
coge_dataObjCreated(*User, *Zone, *DataObjInfo) {
	cyverse_ensureAccessOnCreateDataObj(
		_coge_USER, _coge_COLL, _coge_PERM, *DataObjInfo.logical_path );
}
