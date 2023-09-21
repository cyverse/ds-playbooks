# VERSION: 1
#
# Rules particular to the captcn project

@include "captcn-env"

captcn_isForCapTcn(*Path) = *Path like '/' ++ cyverse_ZONE ++ '/home/shared/CAP_TCN/\*'


captcn_assignPerms(*Path) {
  *type = cyverse_getEntityType(*Path);
  *recursiveFlag = if cyverse_isColl(*type) then 'recursive' else 'default';

  foreach(*user in captcn_OWNERS) {
    msiSetACL(*recursiveFlag, 'own', *user, *Path);
  }

  foreach(*user in captcn_WRITERS) {
    msiSetACL(*recursiveFlag, 'write', *user, *Path);
  }

  foreach(*user in captcn_READERS) {
    msiSetACL(*recursiveFlag, 'read', *user, *Path);
  }

  if (cyverse_isColl(*type)) {
    msiSetACL(*recursiveFlag, 'inherit', 'null', *Path);
  }
}


# Ensure that the correct users have permissions on the CAP_TCN folders that are
# create under the CAP_TCN home
captcn_acPostProcForCollCreate {
  if (captcn_isForCapTcn($collName)) {
    captcn_assignPerms($collName);
  }
}


# Ensure that data objects that are copied into one of the CAP_TCN folders get
# the correct permissions. Copied data objects don't inherit permissions.
captcn_acPostProcForCopy {
  if (captcn_isForCapTcn($objPath) && $writeFlag == 0) {
    captcn_assignPerms($objPath);
  }
}


# Ensure that collections and data objects are moved into a CAP_TCN folder get
# the correct permissions. Moved collections and data objects don't inherit
# permissions.
captcn_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  if (!captcn_isForCapTcn(*SrcEntity) && captcn_isForCapTcn(*DestEntity)) {
    captcn_assignPerms(*DestEntity);
  }
}
