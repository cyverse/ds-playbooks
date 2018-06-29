# PIRE project policy
# include this file from within ipc-custom.re

@include 'pire-env'


_pire_isForPIRE(*Path) = str(*Path) like str(pire_BASE_COLL) ++ '/*'


_pire_ensurePireOwns(*Path) {
  msiGetObjType(*Path, *type);
  *recursiveFlag = if *type == '-c' then 'recursive' else 'default';
  msiSetACL(*recursiveFlag, 'own', 'pire', *Path);
}


# Determines if the provided collection or data object belongs to the PIRE
# project
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the project, otherwise false
#
pire_replBelongsTo : path -> boolean
pire_replBelongsTo(*Entity) = _pire_isForPIRE(*Entity)


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
pire_replIngestResc : string * boolean
pire_replIngestResc = (pire_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
pire_replReplResc : string * boolean
pire_replReplResc = pire_replIngestResc


# Ensure that pire group has write permission on the EHT collections that are
# create under the EHT base collection home
pire_acPostProcForCollCreate {
  if (_pire_isForPIRE($collName)) {
    _pire_ensurePireOwns($collName);
  }
}


# Ensure that data objects that are copied into an EHT collection get the
# correct permissions.
pire_acPostProcForCopy {
  if (_pire_isForPIRE($objPath) && $writeFlag == 0) {
    _pire_ensurePireOwns($objPath);
  }
}


# Ensure that collections and data objects that are moved into an EHT collection
# get the correct permissions.
pire_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  if (!_pire_isForPIRE(*SrcEntity) && _pire_isForPIRE(*DestEntity)) {
    _pire_ensurePireOwns(*DestEntity);
  }
}


# Ensure that data objects that are uploaded into one of the EHT collections get
# the correct permissions.
pire_acPostProcForPut {
  if (_pire_isForPIRE($objPath) && $writeFlag == 0) {
    _pire_ensurePireOwns($objPath);
  }
}
