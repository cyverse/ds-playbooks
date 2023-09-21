# VERSION: 2
#
# Rules particular to the sernec project

@include "sernec-env"

sernec_isForSernec(*Path) = *Path like '/' ++ cyverse_ZONE ++ '/home/shared/sernec/\*'


sernec_assignPerms(*Path, *UserNameClient) {
  *type = ipc_getEntityType(*Path);
  *recursiveFlag = if ipc_isCollection(*type) then 'recursive' else 'default';
  msiSetACL(*recursiveFlag, 'own', *UserNameClient, *Path);

  foreach(*user in sernec_OWNERS) {
    msiSetACL(*recursiveFlag, 'own', *user, *Path);
  }

  foreach(*user in sernec_WRITERS) {
    if (*user != *UserNameClient) {
      msiSetACL(*recursiveFlag, 'write', *user, *Path);
    }
  }

  foreach(*user in sernec_READERS) {
    if (*user != *UserNameClient) {
      msiSetACL(*recursiveFlag, 'read', *user, *Path);
    }
  }

  if (ipc_isCollection(*type)) {
    msiSetACL(*recursiveFlag, 'inherit', 'null', *Path);
  }
}


# Ensure that the correct users have permissions on the sernec folders that are
# create under the sernec home
sernec_acPostProcForCollCreate {
  if (sernec_isForSernec($collName)) {
    sernec_assignPerms($collName, $userNameClient);
  }
}


# Ensure that data objects that are copied into one of the sernec folders get
# the correct permissions. Copied data objects don't inherit permissions.
sernec_acPostProcForCopy {
  if (sernec_isForSernec($objPath) && $writeFlag == 0) {
    sernec_assignPerms($objPath, $userNameClient);
  }
}


# Ensure that collections and data objects are moved into a sernec folder get
# the correct permissions. Moved collections and data objects don't inherit
# permissions.
sernec_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  if (!sernec_isForSernec(*SrcEntity) && sernec_isForSernec(*DestEntity)) {
    sernec_assignPerms(*DestEntity, $userNameClient);
  }
}
