# VERSION 1
#
# ipc-services.re
# This is a library of rules to support service specific policies.

_ipc_HOME = '/' ++ ipc_ZONE ++ '/home'

# This function checks to see if a collection or data object is inside a user
# collection managed by a service.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  SvcColl  the name of the user collection managed by the service
#  Path     the path to the collection or data object of interest
#
# RETURNS:
#  It returns true if the collection or data object is inside the user
#  collection, otherwise it returns false.
#
ipc_isForService: string * string * path -> boolean
ipc_isForService(*SvcUser, *SvcColl, *Path) =
  let *strPath = str(*Path) in
  *strPath like regex _ipc_HOME ++ '/[^/]\*/*SvcColl($|/.\*)'
  && !(*strPath like _ipc_HOME ++ '/*SvcUser/\*')
  && !(*strPath like _ipc_HOME ++ '/shared/\*')


# This rule gives write access to a service for a collection and everything in
# it.
#
# PARAMETERS:
#  SvcUser   the iRODS user name used by the service
#  CollPath  the path to the collection of begin given write access to
#
ipc_giveWriteAccessColl(*SvcUser, *CollPath) {
  writeLine('serverLog', 'permitting *SvcUser write access to *CollPath and everything in it');
  msiSetACL('recursive', 'write', *SvcUser, *CollPath);
}


# This rule gives write access to a service for a data object.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  ObjPath  the path to the data object of begin given write access to
#
ipc_giveWriteAccessObj(*SvcUser, *ObjPath) {
  writeLine('serverLog', 'permitting *SvcUser write access to *ObjPath');
  msiSetACL('default', 'write', *SvcUser, *ObjPath);
}


# This rule ensures that a service user gets write access to a presumably newly
# created collection if it were created inside a user collection managed by the
# service.
#
# PARAMETERS:
#  SvcUser   the iRODS user name used by the service
#  SvcColl   the name of the user collection managed by the service
#  CollPath  the path to the collection of interest
#
ipc_ensureAccessOnCreateColl(*SvcUser, *SvcColl, *CollPath) {
  if (ipc_isForService(*SvcUser, *SvcColl, /*CollPath)) {
    ipc_giveWriteAccessColl(*SvcUser, *CollPath);
  }
}


# This rule ensures that a service user gets write access to a presumably newly
# created data object if it were created inside a user collection managed by the
# service.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  SvcColl  the name of the user collection managed by the service
#  ObjPath  the path to the data object of interest
#
ipc_ensureAccessOnCreateObj(*SvcUser, *SvcColl, *ObjPath) {
  if (ipc_isForService(*SvcUser, *SvcColl, /*ObjPath)) {
    ipc_giveWriteAccessObj(*SvcUser, *ObjPath);
  }
}


# This rule ensures that a service user gets write access to a collection or
# data object if it has been moved into a user collection managed by the
# service.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  SvcColl  the name of the user collection managed by the service
#  OldPath  the original iRODS path to the entity
#  NewPath  the new iRODS path to the entity
#
ipc_ensureAccessOnMv(*SvcUser, *SvcColl, *OldPath, *NewPath) {
  if (!ipc_isForService(*SvcUser, *SvcColl, /*OldPath)
      && ipc_isForService(*SvcUser, *SvcColl, /*NewPath)) {

    msiGetObjType(*NewPath, *type);

    if (*type == '-c') {
      ipc_giveWriteAccessColl(*SvcUser, *NewPath);
    } else if (*type == '-d') {
      ipc_giveWriteAccessObj(*SvcUser, *NewPath);
    }
  }
}
