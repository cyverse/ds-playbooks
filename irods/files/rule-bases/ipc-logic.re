# The iPLANT specific, environment idependent rule customizations.
#
# These rules will be called by the hooks implemented in ipc-custom.re.  The rule names should be
# prefixed with 'ipc' and suffixed with the name of the rule hook that will call the custom rule.

@include 'ipc-amqp'
@include 'ipc-json'
@include 'ipc-uuid'

COLLECTION_TYPE = 'collection'
DATA_OBJECT_TYPE = 'data-object'
RESOURCE_TYPE = 'resource'
USER_TYPE = 'user'


getTimestamp() {
  msiGetSystemTime(*timestamp, 'human')
  *timestamp
}

contains(*item, *list) {
  *result = false;
  foreach (*currItem in *list) {
    if (*item == *currItem) {
      *result = true;
    }
  }
  *result;
}

# Assign a UUID to a given collection or data object.
assignUUID(*ItemType, *ItemName) {
  *uuid = ipc_uuidGenerate;
  writeLine('serverLog', 'UUID *uuid created');
# XXX - This is a workaround for https://github.com/irods/irods/issues/3437. It is still present in
#       4.2.2.
#  msiModAVUMetadata(*ItemType, *ItemName, 'set', 'ipc_UUID', *uuid, '');
  *status = errormsg(msiModAVUMetadata(*ItemType, *ItemName, 'set', 'ipc_UUID', *uuid, ''), *msg);

  if (*status == -818000) {
    # assume it was uploaded by a ticket
    *typeArg = execCmdArg(*ItemType);
    *nameArg = execCmdArg(*ItemName);
    *valArg = execCmdArg(*uuid);
    *argStr = "*typeArg *nameArg *valArg";
    *status = errormsg(msiExecCmd('set-uuid', *argStr, "null", "null", "null", *out), *msg);
    if (*status != 0) {
      writeLine('serverLog', "Failed to assign UUID: *msg");
      fail;
    }
  } else if (*status != 0) {
    writeLine('serverLog', "Failed to assign UUID: *msg");
    fail;
  }
# XXX - ^^^

  *uuid;
}

# Looks up the UUID of a collection from its path.
retrieveCollectionUUID(*Coll) {
  *uuid = '';
  *res = SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *Coll AND META_COLL_ATTR_NAME == 'ipc_UUID';
  foreach (*record in *res) {
    *uuid = *record.META_COLL_ATTR_VALUE;
  }
  *uuid;
}

# Looks up the UUID of a data object from its path.
retrieveDataUUID(*Data) {
  *uuid = '';
  msiSplitPath(*Data, *parentColl, *dataName);
  *res = SELECT META_DATA_ATTR_VALUE
           WHERE COLL_NAME == *parentColl
             AND DATA_NAME == *dataName
             AND META_DATA_ATTR_NAME == 'ipc_UUID';
  foreach (*record in *res) {
    *uuid = *record.META_DATA_ATTR_VALUE;
  }
  *uuid;
}

# Looks up the UUID for a given type of entity (collection or data object)
retrieveUUID(*EntityType, *EntityPath) {
  if (*EntityType == '-c') {
    retrieveCollectionUUID(*EntityPath);
  } else if (*EntityType == '-d') {
    retrieveDataUUID(*EntityPath);
  } else {
    ''
  }
}

sendMsg(*Topic, *Msg) {
  errorcode(ipc_amqpSend(*Topic, *Msg));
  0;
}

mkUserObject(*Field, *Name, *Zone) =
  if (*Zone == '') then
    ipc_jsonObject(*Field, list(ipc_jsonString('name', *Name),
                                ipc_jsonString('zone', $rodsZoneClient)))
  else
    ipc_jsonObject(*Field, list(ipc_jsonString('name', *Name),
                                ipc_jsonString('zone', *Zone)))

mkAuthorField() = mkUserObject('author', $userNameClient, $rodsZoneClient)

mkEntityField(*UUID) = ipc_jsonString('entity', *UUID)

mkPathField(*Path) = ipc_jsonString('path', *Path)

mkAvuObject(*Field, *Name, *Value, *Unit) =
  ipc_jsonObject(*Field, list(ipc_jsonString('attribute', *Name),
                              ipc_jsonString('value', *Value),
                              ipc_jsonString('unit', *Unit)))

getEntityType(*ItemType) =
  match *ItemType with
    | '-c' => COLLECTION_TYPE
    | '-d' => DATA_OBJECT_TYPE
    | '-r' => RESOURCE_TYPE
    | '-u' => USER_TYPE

sendCollectionAdd(*Collection, *Path) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Collection),
                                   mkPathField(*Path)))
  in sendMsg(COLLECTION_TYPE ++ '.add', *msg)

sendDataObjectOpen(*Data) =
  let *msg = ipc_jsonDocument(list(mkEntityField(*Data),
                                   mkPathField($objPath),
                                   mkUserObject('author', $userNameClient, $rodsZoneClient),
                                   ipc_jsonNumber('size', $dataSize),
                                   ipc_jsonString('timestamp', getTimestamp())))
  in sendMsg(DATA_OBJECT_TYPE ++ '.open', *msg)

_ipc_sendDataObjectAdd(*Data, *Path, *Size, *Type) {
  *msg = ipc_jsonDocument(list(mkAuthorField(),
                               mkEntityField(*Data),
                               mkPathField(*Path),
                               mkUserObject('creator', $userNameClient, $rodsZoneClient),
                               ipc_jsonNumber('size', *Size),
                               ipc_jsonString('type', *Type)));

  sendMsg(DATA_OBJECT_TYPE ++ '.add', *msg);
}

_ipc_sendDataObjectAdd(*Data) {
  _ipc_sendDataObjectAdd(*Data, $objPath, $dataSize, $dataType);
}

sendCollectionInheritModified(*Recursive, *Inherit, *Collection) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Collection),
                                   ipc_jsonBoolean('recursive', *Recursive),
                                   ipc_jsonBoolean('inherit', *Inherit)))
  in sendMsg(COLLECTION_TYPE ++ '.acl.mod', *msg)

sendCollectionAclModified(*Recursive, *AccessLevel, *Username, *Zone, *Collection) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Collection),
                                   ipc_jsonBoolean('recursive', *Recursive),
                                   ipc_jsonString('permission', *AccessLevel),
                                   mkUserObject('user', *Username, *Zone)))
  in sendMsg(COLLECTION_TYPE ++ '.acl.mod', *msg)

sendCollectionAccessModified(*Recursive, *AccessLevel, *Username, *Zone, *Collection) {
  if (*AccessLevel == 'inherit') {
    sendCollectionInheritModified(*Recursive, true, *Collection);
  }
  else if (*AccessLevel == 'noinherit') {
    sendCollectionInheritModified(*Recursive, false, *Collection);
  }
  else {
    sendCollectionAclModified(*Recursive, *AccessLevel, *Username, *Zone, *Collection);
  }
}

sendDataObjectAclModified(*AccessLevel, *Username, *Zone, *Data) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Data),
                                   ipc_jsonString('permission', *AccessLevel),
                                   mkUserObject('user', *Username, *Zone)))
  in sendMsg(DATA_OBJECT_TYPE ++ '.acl.mod', *msg)

sendDataObjectMetadataModified(*Data) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(), mkEntityField(*Data)))
  in sendMsg(DATA_OBJECT_TYPE ++ '.sys-metadata.mod', *msg)

sendDataObjectMod(*Data) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Data),
                                   mkUserObject('creator', $dataOwner, $dataOwnerZone),
                                   ipc_jsonNumber('size', $dataSize),
                                   ipc_jsonString('type', $dataType)))
  in sendMsg(DATA_OBJECT_TYPE ++ '.mod', *msg)

sendEntityMove(*EntityType, *Entity, *OldPath, *NewPath) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Entity),
                                   ipc_jsonString('old-path', *OldPath),
                                   ipc_jsonString('new-path', *NewPath)))
  in sendMsg(*EntityType ++ '.mv', *msg)

sendEntityRemove(*EntityType, *Entity, *Path) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Entity),
                                   ipc_jsonString('path', *Path)))
  in sendMsg(*EntityType ++ '.rm', *msg)

sendAvuMod(*ItemType, *Item, *OldName, *OldValue, *OldUnit, *NewName, *NewValue, *NewUnit) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Item),
                                   mkAvuObject('old-metadatum', *OldName, *OldValue, *OldUnit),
                                   mkAvuObject('new-metadatum', *NewName, *NewValue, *NewUnit)))
  in sendMsg(getEntityType(*ItemType) ++ '.metadata.mod', *msg)

sendAvuSet(*Option, *ItemType, *Item, *AName, *AValue, *AUnit) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Item),
                                   mkAvuObject('metadatum', *AName, *AValue, *AUnit)))
  in sendMsg(getEntityType(*ItemType) ++ '.metadata.' ++ *Option, *msg)

sendAvuMultiset(*ItemName, *AName, *AValue, *AUnit) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   ipc_jsonString('pattern', *ItemName),
                                   mkAvuObject('metadatum', *AName, *AValue, *AUnit)))
  in sendMsg(DATA_OBJECT_TYPE ++ '.metadata.addw', *msg)

sendAvuMultiremove(*ItemType, *Item, *AName, *AValue, *AUnit) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   mkEntityField(*Item),
                                   ipc_jsonString('attribute-pattern', *AName),
                                   ipc_jsonString('value-pattern', *AValue),
                                   ipc_jsonString('unit-pattern', *AUnit)))
  in sendMsg(getEntityType(*ItemType) ++ '.metadata.rmw', *msg)

sendAvuCopy(*SourceItemType, *TargetItemType, *Source, *Target) =
  let *msg = ipc_jsonDocument(list(mkAuthorField(),
                                   ipc_jsonString('source', *Source),
                                   ipc_jsonString('source-type', getEntityType(*SourceItemType)),
                                   ipc_jsonString('destination', *Target)))
  in sendMsg(getEntityType(*TargetItemType) ++ '.metadata.cp', *msg)

resolveAdminPerm(*Item) = if *Item like regex '^/[^/]*(/[^/]*)?$' then 'write' else 'own'

setAdminGroupPerm(*Item) {
  msiSetACL('default', resolveAdminPerm(*Item), 'rodsadmin', *Item);
}

canModProtectedAVU(*User) {
  *canMod = false;
  if (*User == 'bisque') {
    *canMod = true;
  } else {
    *res = SELECT USER_ID WHERE USER_NAME = *User AND USER_TYPE = 'rodsadmin';
    foreach (*record in *res) {
      *canMod = true;
      break;
    }
  }
  *canMod;
}

# Gets the original unit for an AVU modification. The argument that is used for the original unit
# in the AVU modification may contain the original unit or, if the unit was empty in the original
# AVU then this argument may contain the first of the new AVU settings instead. We can distinguish
# this case from the others by the presence of a prefix in the value. The prefix is always a single
# character followed by a colon.
#
getOrigUnit(*candidate) =
  if strlen(*candidate) < 2 then *candidate
  else if substr(*candidate, 1, 1) != ':' then *candidate
  else '';

# Gets the new AVU setting from a list of candidates. New AVU settings are passed in an arbitrary
# order and the type of AVU setting is identified by a prefix. This function looks for values
# matching the given prefix. If multiple matching values are found then the last one wins.
#
getNewAVUSetting(*orig, *prefix, *candidates) {
  *setting = *orig
  foreach (*candidate in *candidates) {
    if (strlen(*candidate) >= strlen(*prefix)) {
      if (substr(*candidate, 0, strlen(*prefix)) == *prefix) {
        *setting = substr(*candidate, 2, strlen(*candidate));
      }
    }
  }
  *setting;
}

# Determines whether or not the string in the first argument starts with the string in the second
# argument.
#
startsWith(*str, *prefix) =
  if strlen(*str) < strlen(*prefix) then false
  else if substr(*str, 0, strlen(*prefix)) != *prefix then false
  else true;

# Removes a prefix from a string.
#
removePrefix(*orig, *prefixes) {
  *result = *orig
  foreach (*prefix in *prefixes) {
    if (startsWith(*orig, *prefix)) {
      *result = substr(*orig, strlen(*prefix), strlen(*orig));
      break;
    }
  }
  *result;
}


# The logic for handling a newly created data object.
#
_ipc_createData(*Path) {
  *err = errormsg(msiDataObjChksum(*Path, 'forceChksum=', *chksum), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }
}


# The logic for handling a modified data object.
#
_ipc_modifyData(*Path) {
  *err = errormsg(msiDataObjChksum(*Path, 'forceChksum=', *chksum), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *uuid = retrieveDataUUID(*Path);

  if (*uuid != '') {
    *err = errormsg(sendDataObjectMod(*uuid), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }
  }
}


_ipc_createOrModifyData(*Path, *IsNew) {
  # When an upload is completed after a restart, *IsNew is always false. We need
  # to ensure that rodsadmin always get ownership.
  *err = errormsg(setAdminGroupPerm(*Path), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  if (*IsNew) {
    _ipc_createData(*Path);
  } else {
    _ipc_modifyData(*Path);
  }
}


_ipc_createOrModifyArchiveData(*Path, *IsNew) {
  if (*IsNew) {
    _ipc_sendDataObjectAdd(assignUUID('-d', *Path));
  } else if (retrieveDataUUID(*Path) == '') {
    _ipc_sendDataObjectAdd(assignUUID('-d', *Path));
  }
}


# Indicates whether or not an AVU is protected
avuProtected(*ItemType, *ItemName, *Attribute) {
  if (startsWith(*Attribute, 'ipc')) {
    *Attribute != 'ipc_UUID' || retrieveUUID(*ItemType, *ItemName) != '';
  } else {
    false;
  }
}

# Verifies that an attribute can be modified. If it can't it fails and sends an error message to
# the caller.
ensureAVUEditable(*Editor, *ItemType, *ItemName, *A, *V, *U) {
  if (avuProtected(*ItemType, *ItemName, *A) && !canModProtectedAVU(*Editor)) {
    cut;
    failmsg(-830000, 'CYVERSE ERROR:  attempt to alter protected AVU <*A, *V, *U>');
  }
}

# If an AVU is not protected, it sets the AVU to the given item
setAVUIfUnprotected(*ItemType, *ItemName, *A, *V, *U) {
  if (!avuProtected(*ItemType, *ItemName, *A)) {
    msiModAVUMetadata(*ItemType, *ItemName, 'set', *A, *V, *U);
  }
}


# Copies the unprotected AVUs from a given collection to the given item.
cpUnprotectedCollAVUs(*Coll, *TargetType, *TargetName) =
  foreach (*avu in SELECT META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS
                     WHERE COLL_NAME == *Coll) {
    setAVUIfUnprotected(*TargetType, *TargetName, *avu.META_COLL_ATTR_NAME,
                        *avu.META_COLL_ATTR_VALUE, *avu.META_COLL_ATTR_UNITS);
  }

# Copies the unprotected AVUs from a given data object to the given item.
cpUnprotectedDataObjAVUs(*ObjPath, *TargetType, *TargetName) {
  msiSplitPath(*ObjPath, *parentColl, *objName);
  foreach (*avu in SELECT META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS
                     WHERE COLL_NAME == *parentColl AND DATA_NAME == *objName) {
    setAVUIfUnprotected(*TargetType, *TargetName, *avu.META_DATA_ATTR_NAME,
                        *avu.META_DATA_ATTR_VALUE, *avu.META_DATA_ATTR_UNITS);
  }
}

# Copies the unprotected AVUs from a given resource to the given item.
cpUnprotectedRescAVUs(*Resc, *TargetType, *TargetName) =
  foreach (*avu in SELECT META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
                     WHERE RESC_NAME == *Resc) {
    setAVUIfUnprotected(*TargetType, *TargetName, *avu.META_RESC_ATTR_NAME,
                        *avu.META_RESC_ATTR_VALUE, *avu.META_RESC_ATTR_UNITS);
  }

# Copies the unprotected AVUs from a given user to the given item.
cpUnprotectedUserAVUs(*User, *TargetType, *TargetName) =
  foreach (*avu in SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
                     WHERE USER_NAME == *User) {
    setAVUIfUnprotected(*TargetType, *TargetName, *avu.META_RESC_ATTR_NAME,
                        *avu.META_RESC_ATTR_VALUE, *avu.META_RESC_ATTR_UNITS);
  }


# Perform post proc on bulk uploads
#
ipc_acBulkPutPostProcPolicy {  msiSetBulkPutPostProcPolicy('on'); }

# Create a user for a Data Store service
ipc_acCreateUser {
  msiCreateUser ::: msiRollback;
  msiCommit;
}

# Refuse SSL connections
#
ipc_acPreConnect(*OUT) { *OUT = 'CS_NEG_REFUSE'; }


# Use default threading setting
#
ipc_acSetNumThreads { msiSetNumThreads('default', 'default', 'default'); }


# Set maximum number of rule engine processes
#
ipc_acSetReServerNumProc { msiSetReServerNumProc(str(ipc_MAX_NUM_RE_PROCS)); }


# This rule sets the rodsadin group permission of a collection when a collection
# is created by an administrative means, i.e. iadmin mkuser. It also pushes a
# collection.add message into the irods exchange.
#
ipc_acCreateCollByAdmin(*ParColl, *ChildColl) {
  *coll = '*ParColl/*ChildColl';
  *perm = resolveAdminPerm(*coll);
  msiSetACL('default', 'admin:*perm', 'rodsadmin', *coll);
}

ipc_archive_acCreateCollByAdmin(*ParColl, *ChildColl) {
  *coll = '*ParColl/*ChildColl';
  sendCollectionAdd(assignUUID('-c', *coll), *coll);
}

# This rule pushes a collection.rm message into the irods exchange.
#
ipc_acDeleteCollByAdmin(*ParColl, *ChildColl) {
  *path = '*ParColl/*ChildColl';
  *uuid = retrieveCollectionUUID(*path);
  if (*uuid != '') {
    sendEntityRemove(COLLECTION_TYPE, *uuid, *path);
  }
}


ipc_acPreprocForCollCreate {
  # XXX - Workaround for https://github.com/irods/irods/issues/4060, fixed in iRODS 4.2.7
  if($collName like regex '.*\\\\.*') {
    cut;
    failmsg(-809000, 'Collection name cannot contain a "\"');
  }

  # XXX - Workaround for https://github.com/irods/irods/issues/4750, fixed in iRODS 4.2.8
  if($collName like regex '(.*/|(.*/)?\\.{1,2})') {
    cut;
    failmsg(-809000, 'Collection cannot be named ".", "..", or "/"');
  } else if ($collName like regex '.*//.*') {
    cut;
    failmsg(-809000, 'Path cannot have a "//" in it');
  }
}


# This rule prevents the user from removing rodsadmin's ownership from an ACL unless the user is of
# type rodsadmin.
#
ipc_acPreProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
  if (*UserName == 'rodsadmin') {
    if (!(*AccessLevel like 'admin:*') && *AccessLevel != resolveAdminPerm(*Path)) {
      cut;
      failmsg(-830000, 'CYVERSE ERROR:  attempt to alter admin user permission.');
    }
  }
}


ipc_acPreProcForObjRename(*SourceObject, *DestObject) {
  # XXX - Workaround for https://github.com/irods/irods/issues/4060, fixed in iRODS 4.2.7
  if(*DestObject like regex '.*\\\\.*') {
    cut;
    failmsg(-809000, 'Entity name cannot contain a "\"');
  }

  # XXX - Workaround for https://github.com/irods/irods/issues/4750, fixed in iRODS 4.2.8
  msiGetObjType(*SourceObject, *type);
  if(*type == '-c' && *DestObject like regex '(.*/|(.*/)?\\.{1,2})') {
    cut;
    failmsg(-809000, 'Collection cannot be named ".", "..", or "/"');
  }
}


# This rule makes the admin owner of any created collection.  This rule is not applied to
# collections created when a TAR file is expanded. (i.e. ibun -x)
#
ipc_acPostProcForCollCreate {
  setAdminGroupPerm($collName);
}

# This rule ensures that archival collections are given a UUID and an AMQP
# message is published indicating the collection is created.
#
ipc_archive_acPostProcForCollCreate {
  sendCollectionAdd(assignUUID('-c', $collName), $collName);
}

ipc_acPostProcForOpen {
  *uuid = retrieveDataUUID($objPath);
  if (*uuid != '') { sendDataObjectOpen(*uuid); }
}

ipc_acPreprocForRmColl { temporaryStorage.'$collName' = retrieveCollectionUUID($collName); }

ipc_acPostProcForRmColl {
  *uuid = temporaryStorage.'$collName';
  if (*uuid != '') { sendEntityRemove(COLLECTION_TYPE, *uuid, $collName); }
}

ipc_acDataDeletePolicy { temporaryStorage.'$objPath' = retrieveDataUUID($objPath); }

ipc_acPostProcForDelete {
  *uuid = temporaryStorage.'$objPath';
  if (*uuid != '') { sendEntityRemove(DATA_OBJECT_TYPE, *uuid, $objPath); }
}

# This sends a collection or data-object ACL modification message for the updated object.
#
ipc_acPostProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
  *level = removePrefix(*AccessLevel, list('admin:'));
  msiGetObjType(*Path, *type);
  *uuid = retrieveUUID(*type, *Path);
  if (*uuid != '') {
    if (*type == '-c') {
      sendCollectionAccessModified(bool(*RecursiveFlag), *level, *UserName, *Zone, *uuid);
    } else if (*type == '-d') {
      sendDataObjectAclModified(*level, *UserName, *Zone, *uuid);
    }
  }
}

# This rule sends a system metadata modified status message.
#
ipc_acPostProcForModifyDataObjMeta {
  *uuid = retrieveDataUUID($objPath);
  if (*uuid != '') {
    sendDataObjectMetadataModified(*uuid);
  }
}

# This rule schedules a rename entry job for the data object or collection being renamed.
#
ipc_acPostProcForObjRename(*SrcEntity, *DestEntity) {
  msiGetObjType(*DestEntity, *type);
  *uuid = retrieveUUID(*type, *DestEntity);

  if (*uuid != '') {
    if (*type == '-c') {
      sendEntityMove(COLLECTION_TYPE, *uuid, *SrcEntity, *DestEntity);
    } else if (*type == '-d') {
      sendEntityMove(DATA_OBJECT_TYPE, *uuid, *SrcEntity, *DestEntity);
    }
  } else {
    if (*type == '-c') {
      *err = errormsg(sendCollectionAdd(assignUUID('-c', *DestEntity), *DestEntity), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }
    } else if (*type == '-d') {
      *uuid = assignUUID('-d', *DestEntity);
      msiSplitPath(*DestEntity, *collPath, *dataName);

      *sent = false;

      foreach (*res in select order_asc(DATA_SIZE), DATA_TYPE_NAME
                       where COLL_NAME = '*collPath' and DATA_NAME = '*dataName') {
        if (!*sent) {
          *err = errormsg(
            _ipc_sendDataObjectAdd(*uuid, *DestEntity, *res.DATA_SIZE, *res.DATA_TYPE_NAME),
            *msg);

          if (*err < 0) { writeLine('serverLog', *msg); }
          *sent = true;
        }
      }
    }
  }
}


# This rule ensures that a UUID is assigned to and a checksum is computed for
# every uploaded data object and the rodsadmin group gets own permission.
#
ipc_acPostProcForPut {
  _ipc_createOrModifyData($objPath, $writeFlag == 0);
}


# This rule ensures that a data object change AMQP message is published for
# every uploaded data object.
ipc_archive_acPostProcForPut {
  _ipc_createOrModifyArchiveData($objPath, $writeFlag == 0);
}


# This rule ensures that a UUID is assigned to and a checksum is computed for
# every copied data object and the rodsadmin group gets own permission.
#
ipc_acPostProcForCopy {
  _ipc_createOrModifyData($objPath, $writeFlag == 0);
}


# This rule ensures that a data object change AMQP message is published for
# every uploaded data object.
ipc_archive_acPostProcForCopy {
  _ipc_createOrModifyArchiveData($objPath, $writeFlag == 0);
}


# This rule ensures that every data object created through file registration is
# assigned a UUID and checksum and receives rodsadmin group own permission.
#
ipc_acPostProcForFilePathReg {
  _ipc_createOrModifyData($objPath, true);
}


# This rule ensured that every data object created through fiie registration
# causes a data-object.add AMQP message to be published.
ipc_archive_acPostProcForFilePathReg {
   _ipc_createOrModifyArchiveData($objPath, true);
}


# This rule checks that AVU being modified isn't a protected one.
ipc_acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *New1, *New2,
                                  *New3, *New4) {
  *newArgs = list(*New1, *New2, *New3, *New4);

  # Determine the original unit and the new AVU settings.
  *origUnit = getOrigUnit(*New1);
  *newName = getNewAVUSetting(*AName, 'n:', *newArgs);
  *newValue = getNewAVUSetting(*AValue, 'v:', *newArgs);
  *newUnit = getNewAVUSetting(*origUnit, 'u:', *newArgs);

  ensureAVUEditable($userNameClient, *ItemType, *ItemName, *AName, *AValue, *origUnit);
  ensureAVUEditable($userNameClient, *ItemType, *ItemName, *newName, *newValue, *newUnit);
}

# This rule checks that AVU being added, set or removed isn't a protected one.
# Only rodsadmin users are allowed to add, remove or update protected AVUs.
ipc_acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
  if (*Option == 'add' || *Option == 'addw') {
    ensureAVUEditable($userNameProxy, *ItemType, *ItemName, *AName, *AValue, *AUnit);
  } else if (*Option == 'set') {
    if (*ItemType == '-c') {
      *query =
        SELECT META_COLL_ATTR_ID WHERE COLL_NAME == *ItemName AND META_COLL_ATTR_NAME == *AName;
    } else if (*ItemType == '-d') {
      msiSplitPath(*ItemName, *collPath, *dataName);

      *query =
        SELECT META_DATA_ATTR_ID
        WHERE COLL_NAME == *collPath AND DATA_NAME == *dataName AND META_DATA_ATTR_NAME == *AName;
    } else if (*ItemType == '-r') {
      *query =
        SELECT META_RESC_ATTR_ID WHERE RESC_NAME == *ItemName AND META_RESC_ATTR_NAME == *AName;
    } else if (*ItemType == '-u') {
      *query =
        SELECT META_USER_ATTR_ID WHERE USER_NAME == *ItemName AND META_USER_ATTR_NAME == *AName;
    } else {
      writeLine('serverLog', 'unknown imeta item type "*ItemType"');
      fail;
    }

    *exists = false;

    foreach (*record in *query) {
      *exists = true;
      break;
    }

    *authenticatee = if *exists then $userNameClient else $userNameProxy;
    ensureAVUEditable(*authenticatee, *ItemType, *ItemName, *AName, *AValue, *AUnit);
  } else if (*Option == 'rm' || *Option == 'rmw') {
    ensureAVUEditable($userNameClient, *ItemType, *ItemName, *AName, *AValue, *AUnit);
  } else if (*Option != 'adda') {
    writeLine('serverLog', 'unknown imeta option "*Option"');
  }
}

# This rule ensures that only the non-protected AVUs are copied from one item to another.
ipc_acPreProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                                  *TargetItemName) {
  if (!canModProtectedAVU($userNameClient)) {
    if (*SourceItemType == '-c') {
      cpUnprotectedCollAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
    } else if (*SourceItemType == '-d') {
      cpUnprotectedDataObjAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
    } else if (*SourceItemType == '-r') {
      cpUnprotectedRescAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
    } else if (*SourceItemType == '-u') {
      cpUnprotectedUserAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
    }

    # fail to prevent iRODS from also copying the protected metadata
    cut;
    failmsg(0, 'CYVERSE SUCCESS:  Successfully copied the unprotected metadata.');
  }
}

# This rule sends a message indicating that an AVU was modified.
#
ipc_acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *new1,
                                   *new2, *new3, *new4) {
  *uuid = retrieveUUID(*ItemType, *ItemName);
  if (*uuid != '') {
    *newArgs = list(*new1, *new2, *new3, *new4);

    # Determine the original unit and the new AVU settings.
    *origUnit = getOrigUnit(*new1);
    *newName = getNewAVUSetting(*AName, 'n:', *newArgs);
    *newValue = getNewAVUSetting(*AValue, 'v:', *newArgs);
    *newUnit = getNewAVUSetting(*origUnit, 'u:', *newArgs);

    # Send AVU modified message.
    sendAvuMod(*ItemType, *uuid, *AName, *AValue, *origUnit, *newName, *newValue, *newUnit);
  }
}

# This rule sends one of the AVU metadata set messages, depending on which subcommand was used.
#
ipc_acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
  if (*AName != 'ipc_UUID') {
    if (contains(*Option, list('add', 'adda', 'rm', 'set'))) {
      *uuid = retrieveUUID(*ItemType, *ItemName);
      if (*uuid != '') {
        sendAvuSet(*Option, *ItemType, *uuid, *AName, *AValue, *AUnit);
      }
    } else if (*Option == 'addw') {
      sendAvuMultiset(*ItemName, *AName, *AValue, *AUnit);
    } else if (*Option == 'rmw') {
      *uuid = retrieveUUID(*ItemType, *ItemName);
      if (*uuid != '') {
        sendAvuMultiremove(*ItemType, *uuid, *AName, *AValue, *AUnit)
      }
    }
  }
}


# This rules sends an AVU metadata copy message.
#
ipc_acPostProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                                   *TargetItemName) {
  *source = match *SourceItemType with
              | '-c' => retrieveCollectionUUID(*SourceItemName)
              | '-d' => retrieveDataUUID(*SourceItemName)
              | '-r' => *SourceItemName
              | '-u' => *SourceItemName;

  *target = match *TargetItemType with
              | '-c' => retrieveCollectionUUID(*TargetItemName)
              | '-d' => retrieveDataUUID(*TargetItemName)
              | '-r' => *TargetItemName
              | '-u' => *TargetItemName;

  if (*source != '' && *target != '') {
    sendAvuCopy(*SourceItemType, *TargetItemType, *Source, *Target);
  }
}


# Whenever a large file is uploaded, recheck the free space on the storage
# resource server where the file was written.
#
ipc_acPostProcForParallelTransferReceived(*LeafResource) {
  msi_update_unixfilesystem_resource_free_space(*LeafResource);
}


# XXX - Workaround for https://github.com/irods/irods/issues/4060, fixed in iRODS 4.2.7
ipc_acSetRescSchemeForCreate {
  if($objPath like regex '.*\\\\.*') {
    cut;
    failmsg(-809000, 'Data object name cannot contain a "\"');
  }
}
