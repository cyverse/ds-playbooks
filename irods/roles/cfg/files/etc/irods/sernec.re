# VERSION: 1
#
# Rules particular to the sernec project
# include this file from with ipc-custom.re

@include "sernec-env"

sernec_isForSernec(*Path) = *Path like '/iplant/home/shared/sernec/\*'


sernec_assignPerms(*Path) {
  msiGetObjType(*Path, *type);
  *recursiveFlag = if *type == '-c' then 'recursive' else 'default';
  
  foreach(*user in sernec_OWNERS) {
    msiSetACL(*recursiveFlag, 'own', *user, *Path);
  }

  foreach(*user in sernec_WRITERS) {
    msiSetACL(*recursiveFlag, 'write', *user, *Path);
  }

  foreach(*user in sernec_READERS) {
    msiSetACL(*recursiveFlag, 'read', *user, *Path);
  }

  if (*type == '-c') {
    msiSetACL(*recursiveFlag, 'inherit', 'null', *Path);
  }
}


# Ensure that the correct users have permissions on the sernec folders that are 
# create under the sernec home
sernec_acPostProcForCollCreate {
  if (sernec_isForSernec($collName)) {
    sernec_assignPerms($collName);
  }
}


# Ensure that data objects that are copied into one of the sernec folders get
# the correct permissions. Copied data objects don't inherit permissions.
sernec_acPostProcForCopy {
  if (sernec_isForSernec($objPath) && $writeFlag == 0) {
    sernec_assignPerms($objPath);
  }
}


# Ensure that collections and data objects are moved into a sernec folder get
# the correct permissions. Moved collections and data objects don't inherit 
# permissions.
sernec_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  if (!sernec_isForSernec(*SrcEntity) && sernec_isForSernec(*DestEntity)) {
    sernec_assignPerms(*DestEntity);
  }
}
