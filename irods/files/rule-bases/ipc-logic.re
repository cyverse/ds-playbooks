# The general Data Store rule independent of environment specific rule
# customizations.
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

@include 'cyverse_json'

#
# LISTS
#

_ipc_contains(*Item, *List) =
	if size(*List) == 0 then false
	else if *Item == hd(*List) then true
	else _ipc_contains(*Item, tl(*List))


#
# STRINGS
#

# Determines whether or not the string in the first argument starts with the
# string in the second argument.
_ipc_startsWith(*Str, *Prefix) =
	if strlen(*Str) < strlen(*Prefix) then false
	else if substr(*Str, 0, strlen(*Prefix)) != *Prefix then false
	else true

# Removes a prefix from a string.
_ipc_removePrefix(*Orig, *Prefixes) =
	if size(*Prefixes) == 0 then *Orig
	else
		if _ipc_startsWith(*Orig, hd(*Prefixes))
		then substr(*Orig, strlen(hd(*Prefixes)), strlen(*Orig))
		else _ipc_removePrefix(*Orig, tl(*Prefixes))


#
# ICAT IDS
#

_ipc_getCollectionId(*Path) =
	let *id = -1 in
	let *_ = foreach (*rec in SELECT COLL_ID WHERE COLL_NAME = *Path) { *id = int(*rec.COLL_ID) } in
	*id

_ipc_getDataId(*Path) =
	let *collPath = '' in
	let *dataName = '' in
	let *_ = msiSplitPath(*Path, *collPath, *dataName) in
	let *id = -1 in
	let *_ = foreach ( *rec in
			SELECT DATA_ID WHERE COLL_NAME = *collPath AND DATA_NAME = *dataName
		) { *id = int(*rec.DATA_ID) } in
	*id

_ipc_getEntityId(*Path) =
	if ipc_isCollection(ipc_getEntityType(*Path)) then _ipc_getCollectionId(*Path)
	else _ipc_getDataId(*Path)


#
# USER INFO
#

_ipc_isAdmin(*UserName, *UserZone) =
	let *admin = false in
	let *_ = foreach ( *rec in
			SELECT USER_ID
			WHERE USER_NAME = *UserName AND USER_ZONE = *UserZone AND USER_TYPE = 'rodsadmin'
		) { *admin = true } in
	*admin


#
# AVUS
#

# Gets the original unit for an AVU modification. The argument that is used for
# the original unit in the AVU modification may contain the original unit or, if
# the unit was empty in the original AVU then this argument may contain the
# first of the new AVU settings instead. We can distinguish this case from the
# others by the presence of a prefix in the value. The prefix is always a single
# character followed by a colon.
#
_ipc_getOrigUnit(*Candidate) =
	if strlen(*Candidate) < 2 then *Candidate
	else if substr(*Candidate, 1, 1) != ':' then *Candidate
	else ''

# Gets the new AVU setting from a list of candidates. New AVU settings are
# passed in an arbitrary order and the type of AVU setting is identified by a
# prefix. This function looks for values matching the given prefix. If multiple
# matching values are found then the last one wins.
#
_ipc_getNewAVUSetting(*Orig, *Prefix, *Candidates) =
	if size(*Candidates) == 0 then *Orig
	else
		let *candidate = hd(*Candidates) in
		if _ipc_startsWith(*candidate, *Prefix)
		then substr(*candidate, 2, strlen(*candidate))
		else _ipc_getNewAVUSetting(*Orig, *Prefix, tl(*Candidates))


#
# CHECKSUMS
#

# Compute the checksum of of a given replica of a given data object
_ipc_chksumRepl(*Object, *ReplNum) {
	*opt = '';
	msiAddKeyValToMspStr('forceChksum', '', *opts);
	msiAddKeyValToMspStr('replNum', str(*ReplNum), *opts);
	msiDataObjChksum(*Object, *opts, *_);
}


#
# UUIDS
#

_ipc_UUID_ATTR = 'ipc_UUID'

# Looks up the UUID of a collection from its path.
_ipc_retrieveCollectionUUID(*Coll) =
	let *coll = str(*Coll) in
	let *uuid = '' in
	let *attr = _ipc_UUID_ATTR in
	let *_ = foreach ( *record in
			SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *coll AND META_COLL_ATTR_NAME == *attr
		) { *uuid = *record.META_COLL_ATTR_VALUE; } in
	*uuid

# Looks up the UUID of a data object from its path.
_ipc_retrieveDataUUID(*Data) =
	let *parentColl = '' in
	let *dataName = '' in
	let *_ = msiSplitPath(*Data, *parentColl, *dataName) in
	let *uuid = '' in
	let *attr = _ipc_UUID_ATTR in
	let *_ = foreach ( *record in
			SELECT META_DATA_ATTR_VALUE
			WHERE COLL_NAME == *parentColl AND DATA_NAME == *dataName AND META_DATA_ATTR_NAME == *attr
		) { *uuid = *record.META_DATA_ATTR_VALUE; } in
	*uuid

# Looks up the UUID for a given type of entity (collection or data object)
_ipc_retrieveUUID(*EntityType, *EntityPath) =
	if ipc_isCollection(*EntityType) then _ipc_retrieveCollectionUUID(*EntityPath)
	else if ipc_isDataObject(*EntityType) then _ipc_retrieveDataUUID(*EntityPath)
	else ''

_ipc_generateUUID(*UUID) {
	*status = errorcode(msiExecCmd("generateuuid", "", "null", "null", "null", *out));
	if (*status == 0) {
		msiGetStdoutInExecCmdOut(*out, *uuid);
		*UUID = trimr(*uuid, "\n");
		writeLine('serverLog', 'UUID *UUID created');
	} else {
		writeLine("serverLog", "failed to generate UUID");
		fail;
	}
}

# Assign a UUID to a given collection or data object.
_ipc_assignUUID(*ItemType, *ItemName, *Uuid, *ClientName, *ClientZone) {
	*status = 0;

	if (_ipc_isAdmin(*ClientName, *ClientZone)) {
		*status = errormsg(
			msiModAVUMetadata(*ItemType, *ItemName, 'set', _ipc_UUID_ATTR, *Uuid, ''), *msg );

		if (*status != 0) {
			writeLine('serverLog', "DS: Failed to assign UUID to *ItemName");
			writeLine('serverLog', "DS: *msg");
			fail;
		}
	} else {
		ipc_setProtectedAVU(*ItemName, _ipc_UUID_ATTR, *Uuid, '');
	}
}

_ipc_ensureUUID(*EntityType, *EntityPath, *UUID, *ClientName, *ClientZone) {
	*uuid = _ipc_retrieveUUID(*EntityType, *EntityPath);
	if (*uuid == '') {
		_ipc_generateUUID(*uuid);
		_ipc_assignUUID(*EntityType, *EntityPath, *uuid, *ClientName, *ClientZone);
	}
	*UUID = *uuid;
}


#
# ACTION TRACKING
#

_ipc_mkActionKey(*EntityId) = str(*EntityId) ++ '-ROOT_ACTION'

_ipc_isCurrentAction(*EntityId, *Action) =
	let *key = _ipc_mkActionKey(*EntityId) in temporaryStorage."*key" == *Action

_ipc_registerAction(*EntityId, *Action) {
	*key = _ipc_mkActionKey(*EntityId);
	if (if errorcode(temporaryStorage."*key") != 0 then true else temporaryStorage."*key" == '') {
		temporaryStorage."*key" = *Action;
	}
}

_ipc_unregisterAction(*EntityId, *Action) {
	*key = _ipc_mkActionKey(*EntityId);
	if (temporaryStorage."*key" == *Action) {
		temporaryStorage."*key" = '';
	}
}


#
# MESSAGE PUBLISHING
#

_ipc_COLL_MSG_TYPE = 'collection'
_ipc_DATA_MSG_TYPE = 'data-object'
_ipc_RESC_MSG_TYPE = 'resource'
_ipc_USER_MSG_TYPE = 'user'

_ipc_getMsgType(*ItemType) =
	if ipc_isCollection(*ItemType) then _ipc_COLL_MSG_TYPE
	else if ipc_isDataObject(*ItemType) then _ipc_DATA_MSG_TYPE
	else if ipc_isResource(*ItemType) then _ipc_RESC_MSG_TYPE
	else if ipc_isUser(*ItemType) then _ipc_USER_MSG_TYPE
	else ''

_ipc_mkAVUObject(*Field, *Name, *Value, *Unit) =
	cyverse_json_object(
		*Field,
		list(
			cyverse_json_string('attribute', *Name),
			cyverse_json_string('value', *Value),
			cyverse_json_string('unit', *Unit) ) )

_ipc_mkEntityField(*Uuid) = cyverse_json_string('entity', *Uuid)

_ipc_mkPathField(*Path) = cyverse_json_string('path', "*Path")

_ipc_mkUserObject(*Field, *Name, *Zone) = cyverse_json_object(
	*Field, list(cyverse_json_string('name', *Name), cyverse_json_string('zone', *Zone)) )

_ipc_mkAuthorField(*Name, *Zone) = _ipc_mkUserObject('author', *Name, *Zone)

_ipc_resolve_msg_entity_id(*EntityType, *EntityName, *ClientName, *ClientZone) =
	let *id = '' in
	let *_ =
		if ipc_isFileSystemType(*EntityType)
		then _ipc_ensureUUID(*EntityType, *EntityName, *id, *ClientName, *ClientZone)
		else *id = *EntityName in
	*id

# sends a message to a given AMQP topic exchange
#
# Parameters:
# *Topic (string) the topic of the message
# *Msg (string) the message to send
#
# Remote Execution:
# It executes the amqptopicsend.py command script on the rule engine host
#
sendMsg(*Topic, *Msg) {
	*exchangeArg = execCmdArg(cyverse_AMQP_EXCHANGE);
	*topicArg = execCmdArg(*Topic);
	*msgArg = execCmdArg(*Msg);
	*argStr = '*exchangeArg *topicArg *msgArg';

	*status = errormsg(msiExecCmd('amqptopicsend.py', *argStr, cyverse_RE_HOST, '', 0, *out), *msg);

	if (*status < 0) {
		msiGetStderrInExecCmdOut(*out, *err);
		writeLine("serverLog", "Failed to send AMQP message: *msg");
		writeLine("serverLog", *err);
	}

	0;
}

_ipc_sendCollectionAclModified(
	*Collection, *AccessLevel, *UserName, *UserZone, *Recursive, *AuthorName, *AuthorZone )
{
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Collection),
			cyverse_json_boolean('recursive', *Recursive),
			cyverse_json_string('permission', *AccessLevel),
			_ipc_mkUserObject('user', *UserName, *UserZone) ) );

	sendMsg(_ipc_COLL_MSG_TYPE ++ '.acl.mod', *msg);
}

_ipc_sendCollectionInheritModified(*Collection, *Inherit, *Recursive, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Collection),
			cyverse_json_boolean('recursive', *Recursive),
			cyverse_json_boolean('inherit', *Inherit) ) );

	sendMsg(_ipc_COLL_MSG_TYPE ++ '.acl.mod', *msg);
}

_ipc_sendCollectionAccessModified(
	*Collection, *AccessLevel, *UserName, *UserZone, *Recursive, *AuthorName, *AuthorZone )
{
	if (*AccessLevel == 'inherit') {
		_ipc_sendCollectionInheritModified(*Collection, true, *Recursive, *AuthorName, *AuthorZone);
	}
	else if (*AccessLevel == 'noinherit') {
		_ipc_sendCollectionInheritModified(*Collection, false, *Recursive, *AuthorName, *AuthorZone);
	}
	else {
		_ipc_sendCollectionAclModified(
			*Collection, *AccessLevel, *UserName, *UserZone, *Recursive, *AuthorName, *AuthorZone );
	}
}

_ipc_sendCollectionAdd(*Id, *Path, *CreatorName, *CreatorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*CreatorName, *CreatorZone),
			_ipc_mkEntityField(*Id),
			_ipc_mkPathField(*Path) ) );

	sendMsg(_ipc_COLL_MSG_TYPE ++ '.add', *msg);
}

_ipc_sendDataObjectAclModified(*Data, *AccessLevel, *UserName, *UserZone, *AuthorName, *AuthorZone)
{
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Data),
			cyverse_json_string('permission', *AccessLevel),
			_ipc_mkUserObject('user', *UserName, *UserZone) ) );

	sendMsg(_ipc_DATA_MSG_TYPE ++ '.acl.mod', *msg);
}

_ipc_sendDataObjectAdd(
	*AuthorName, *AuthorZone, *Data, *Path, *OwnerName, *OwnerZone, *Size, *Type
) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Data),
			_ipc_mkPathField(*Path),
			_ipc_mkUserObject('creator', *OwnerName, *OwnerZone),
			cyverse_json_number('size', *Size),
			cyverse_json_string('type', *Type) ) );

	sendMsg(_ipc_DATA_MSG_TYPE ++ '.add', *msg);
}

# Publish a data-object.mod message to AMQP exchange
_ipc_sendDataObjectMod(
	*AuthorName, *AuthorZone, *Object, *Path, *OwnerName, *OwnerZone, *Size, *Type
) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Object),
			_ipc_mkUserObject('creator', *OwnerName, *OwnerZone),
			cyverse_json_number('size', *Size),
			cyverse_json_string('type', *Type) ) );

	sendMsg(_ipc_DATA_MSG_TYPE ++ '.mod', *msg);
}

_ipc_sendDataObjectOpen(*Id, *Path, *CreatorName, *CreatorZone, *Size) {
	if (*CreatorName != 'anonymous') {
		msiGetSystemTime(*timestamp, 'human');

		*msg = cyverse_json_document(
			list(
				_ipc_mkAuthorField(*CreatorName, *CreatorZone),
				_ipc_mkEntityField(*Id),
				_ipc_mkPathField(*Path),
				cyverse_json_number('size', *Size),
				cyverse_json_string('timestamp', *timestamp) ) );

		sendMsg(_ipc_DATA_MSG_TYPE ++ '.open', *msg);
	}
}

# Publish a data-object.sys-metadata.mod message to AMQP exchange
_ipc_sendDataObjectMetadataModified(*Data, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Data) ) );

	sendMsg(_ipc_DATA_MSG_TYPE ++ '.sys-metadata.mod', *msg);
}

_ipc_sendAvuMultiset(*ItemName, *AName, *AValue, *AUnit, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			cyverse_json_string('pattern', *ItemName),
			_ipc_mkAVUObject('metadatum', *AName, *AValue, *AUnit) ) );

	sendMsg(_ipc_DATA_MSG_TYPE ++ '.metadata.addw', *msg);
}

_ipc_sendAvuCopy(*SourceItemType, *Source, *TargetItemType, *Target, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			cyverse_json_string('source', *Source),
			cyverse_json_string('source-type', _ipc_getMsgType(*SourceItemType)),
			cyverse_json_string('destination', *Target) ) );

	sendMsg(_ipc_getMsgType(*TargetItemType) ++ '.metadata.cp', *msg);
}

_ipc_sendAvuMod(
	*ItemType,
	*Item,
	*OldName,
	*OldValue,
	*OldUnit,
	*NewName,
	*NewValue,
	*NewUnit,
	*AuthorName,
	*AuthorZone )
{
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Item),
			_ipc_mkAVUObject('old-metadatum', *OldName, *OldValue, *OldUnit),
			_ipc_mkAVUObject('new-metadatum', *NewName, *NewValue, *NewUnit) ) );

	sendMsg(_ipc_getMsgType(*ItemType) ++ '.metadata.mod', *msg);
}

_ipc_sendAvuMultiremove(*ItemType, *Item, *AName, *AValue, *AUnit, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Item),
			cyverse_json_string('attribute-pattern', *AName),
			cyverse_json_string('value-pattern', *AValue),
			cyverse_json_string('unit-pattern', *AUnit) ) );

	sendMsg(_ipc_getMsgType(*ItemType) ++ '.metadata.rmw', *msg);
}

_ipc_sendAvuSet(*Option, *ItemType, *Item, *AName, *AValue, *AUnit, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Item),
			_ipc_mkAVUObject('metadatum', *AName, *AValue, *AUnit) ) );

	sendMsg(_ipc_getMsgType(*ItemType) ++ '.metadata.' ++ *Option, *msg);
}

_ipc_sendEntityMove(*Type, *Id, *OldPath, *NewPath, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Id),
			cyverse_json_string('old-path', '*OldPath'),
			cyverse_json_string('new-path', '*NewPath') ) );

	sendMsg(_ipc_getMsgType(*Type) ++ '.mv', *msg);
}

_ipc_sendEntityRemove(*Type, *Id, *Path, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_ipc_mkAuthorField(*AuthorName, *AuthorZone),
			_ipc_mkEntityField(*Id),
			_ipc_mkPathField(*Path) ) );

	sendMsg(_ipc_getMsgType(*Type) ++ '.rm', *msg);
}


#
# PROTECTED AVUS
#

# Indicates whether or not an AVU is protected
_ipc_avuProtected(*Attribute) = _ipc_startsWith(*Attribute, 'ipc')

# Verifies that an attribute can be modified. If it can't it fails and sends an
# error message to the caller.
_ipc_ensureAVUEditable(*EditorName, *EditorZone, *A, *V, *U) {
	if (_ipc_avuProtected(*A) && !_ipc_isAdmin(*EditorName, *EditorZone)) {
		cut;
		failmsg(-830000, 'CYVERSE ERROR: attempt to alter protected AVU <*A, *V, *U>');
	}
}

# If an AVU is not protected, it sets the AVU to the given item
_ipc_setAVUIfUnprotected(*ItemType, *ItemName, *A, *V, *U) {
	if (!_ipc_avuProtected(*A)) {
		msiModAVUMetadata(*ItemType, *ItemName, 'set', *A, *V, *U);
	}
}

# Copies the unprotected AVUs from a given collection to the given item.
_ipc_cpUnprotectedCollAVUs(*Coll, *TargetType, *TargetName) {
	foreach (*avu in
		SELECT META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS
		WHERE COLL_NAME == *Coll
	) {
		_ipc_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_COLL_ATTR_NAME,
			*avu.META_COLL_ATTR_VALUE,
			*avu.META_COLL_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given data object to the given item.
_ipc_cpUnprotectedDataObjAVUs(*ObjPath, *TargetType, *TargetName) {
	msiSplitPath(*ObjPath, *parentColl, *objName);

	foreach (*avu in
		SELECT META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS
		WHERE COLL_NAME == *parentColl AND DATA_NAME == *objName
	) {
		_ipc_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_DATA_ATTR_NAME,
			*avu.META_DATA_ATTR_VALUE,
			*avu.META_DATA_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given resource to the given item.
_ipc_cpUnprotectedRescAVUs(*Resc, *TargetType, *TargetName) {
	foreach (*avu in
		SELECT META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
		WHERE RESC_NAME == *Resc
	) {
		_ipc_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_RESC_ATTR_NAME,
			*avu.META_RESC_ATTR_VALUE,
			*avu.META_RESC_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given user to the given item.
_ipc_cpUnprotectedUserAVUs(*User, *TargetType, *TargetName) {
	foreach (*avu in
		SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
		WHERE USER_NAME == *User
	) {
		_ipc_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_USER_ATTR_NAME,
			*avu.META_USER_ATTR_VALUE,
			*avu.META_USER_ATTR_UNITS );
	}
}


#
# RODSADMIN GROUP PERMISSIONS
#

resolveAdminPerm(*Item) =
	if *Item like regex '/' ++ cyverse_ZONE ++ '(/trash)?/home/.*' then 'own' else 'write'

setAdminGroupPerm(*Item) {
	msiSetACL('default', resolveAdminPerm(*Item), 'rodsadmin', *Item);
}


#
# STATIC PEPS
#

# Create a user for a Data Store service
ipc_acCreateUser {
	msiCreateUser ::: msiRollback;
	msiCommit;
}

# Refuse SSL connections
ipc_acPreConnect(*OUT) { *OUT = 'CS_NEG_REFUSE'; }

# Use default threading setting
ipc_acSetNumThreads { msiSetNumThreads('default', 'default', 'default'); }

# Set maximum number of rule engine processes
ipc_acSetReServerNumProc { msiSetReServerNumProc(str(cyverse_MAX_NUM_RE_PROCS)); }

# This rule sets the rodsadmin group permission of a collection when a collection
# is created by an administrative means, i.e. iadmin mkuser. It also pushes a
# collection.add message into the irods exchange.
ipc_acCreateCollByAdmin(*ParColl, *ChildColl) {
	*coll = '*ParColl/*ChildColl';
	*perm = resolveAdminPerm(*coll);
	msiSetACL('default', 'admin:*perm', 'rodsadmin', *coll);
}

ipc_archive_acCreateCollByAdmin(*ParColl, *ChildColl) {
	*path = *ParColl ++ '/' ++ *ChildColl;
	*id = '';
	_ipc_ensureUUID(ipc_COLLECTION, *path, *id, $userNameClient, $rodsZoneClient);
	_ipc_sendCollectionAdd(*id, *path, $userNameClient, $rodsZoneClient);
}

# This rule pushes a collection.rm message into the irods exchange.
ipc_acDeleteCollByAdmin(*ParColl, *ChildColl) {
	*path = '*ParColl/*ChildColl';
	*uuid = _ipc_retrieveCollectionUUID(*path);

	if (*uuid != '') {
		_ipc_sendEntityRemove(ipc_COLLECTION, *uuid, *path, $userNameClient, $rodsZoneClient);
	}
}

# This rule prevents the user from removing rodsadmin's ownership from an ACL
# unless the user is of type rodsadmin.
ipc_acPreProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
	if (*UserName == 'rodsadmin') {
		if (!(*AccessLevel like 'admin:*') && *AccessLevel != resolveAdminPerm(*Path)) {
			cut;
			failmsg(-830000, 'CYVERSE ERROR: attempt to alter admin user permission.');
		}
	}
}

# This rule makes the admin owner of any created collection. This rule is not
# applied to collections created when a TAR file is expanded. (i.e. ibun -x)
ipc_acPostProcForCollCreate { setAdminGroupPerm($collName); }

# This rule ensures that archival collections are given a UUID and an AMQP
# message is published indicating the collection is created.
ipc_archive_acPostProcForCollCreate {
	*id = '';
	_ipc_ensureUUID(ipc_COLLECTION, $collName, *id, $userNameClient, $rodsZoneClient);
	_ipc_sendCollectionAdd(*id, $collName, $userNameClient, $rodsZoneClient);
}


# This rule ensures that the storage resource free space is updated when a
# data object is replicated to it.
#
ipc_acPostProcForDataCopyReceived(*leaf_resource) {
   msi_update_unixfilesystem_resource_free_space(*leaf_resource);
}


ipc_acPostProcForOpen {
	*me = 'ipc_acPostProcForOpen';
	*id = _ipc_getDataId($objPath);

	if (*id >= 0) {
		_ipc_registerAction(*id, *me);

		if (_ipc_isCurrentAction(*id, *me)) {
			*uuid = '';
			_ipc_ensureUUID(ipc_DATA_OBJECT, $objPath, *uuid, $userNameClient, $rodsZoneClient);
			_ipc_sendDataObjectOpen(*uuid, $objPath, $userNameClient, $rodsZoneClient, $dataSize);
			_ipc_unregisterAction(*id, *me);
		}
	}
}

ipc_acPreprocForRmColl { temporaryStorage.'$collName' = 	_ipc_retrieveCollectionUUID($collName); }

ipc_acPostProcForRmColl {
	*uuid = temporaryStorage.'$collName';

	if (*uuid != '') {
		_ipc_sendEntityRemove(ipc_COLLECTION, *uuid, $collName, $userNameClient, $rodsZoneClient);
	}
}

ipc_acDataDeletePolicy { temporaryStorage.'$objPath' = _ipc_retrieveDataUUID($objPath); }

ipc_acPostProcForDelete {
	*uuid = temporaryStorage.'$objPath';

	if (*uuid != '') {
		_ipc_sendEntityRemove(ipc_DATA_OBJECT, *uuid, $objPath, $userNameClient, $rodsZoneClient);
	}
}

# This sends a collection or data-object ACL modification message for the
# updated object.
ipc_acPostProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *UserZone, *Path) {
	*me = 'ipc_acPostProcForModifyAccessControl';
	*entityId = _ipc_getEntityId(*Path);

	if (*entityId >= 0) {
		_ipc_registerAction(*entityId, *me);

		if (_ipc_isCurrentAction(*entityId, *me)) {
			*level = _ipc_removePrefix(*AccessLevel, list('admin:'));
			*type = ipc_getEntityType(*Path);
			*userZone = if *UserZone == '' then cyverse_ZONE else *UserZone;
			*uuid = '';
			_ipc_ensureUUID(*type, *Path, *uuid, $userNameClient, $rodsZoneClient);

			if (ipc_isCollection(*type)) {
				_ipc_sendCollectionAccessModified(
					*uuid,
					*level,
					*UserName,
					*userZone,
					bool(*RecursiveFlag),
					$userNameClient,
					$rodsZoneClient );
			} else if (ipc_isDataObject(*type)) {
				_ipc_sendDataObjectAclModified(
					*uuid, *level, *UserName, *userZone, $userNameClient, $rodsZoneClient );
			}

			_ipc_unregisterAction(*entityId, *me);
		}
	}
}

# This rule schedules a rename entry job for the data object or collection being
# renamed.
ipc_acPostProcForObjRename(*SrcEntity, *DestEntity) {
	*type = ipc_getEntityType(*DestEntity);
	*uuid = '';
	_ipc_ensureUUID(*type, *DestEntity, *uuid, $userNameClient, $rodsZoneClient);

	if (*uuid != '') {
		_ipc_sendEntityMove(*type, *uuid, *SrcEntity, *DestEntity, $userNameClient, $rodsZoneClient);
	}
}

# This rule checks that AVU being modified isn't a protected one.
ipc_acPreProcForModifyAVUMetadata(
	*Option, *ItemType, *ItemName, *AName, *AValue, *New1, *New2, *New3, *New4
) {
	*newArgs = list(*New1, *New2, *New3, *New4);

	# Determine the original unit and the new AVU settings.
	*origUnit = _ipc_getOrigUnit(*New1);
	*newName = _ipc_getNewAVUSetting(*AName, 'n:', *newArgs);
	*newValue = _ipc_getNewAVUSetting(*AValue, 'v:', *newArgs);
	*newUnit = _ipc_getNewAVUSetting(*origUnit, 'u:', *newArgs);

	_ipc_ensureAVUEditable($userNameClient, $rodsZoneClient, *AName, *AValue, *origUnit);
	_ipc_ensureAVUEditable($userNameClient, $rodsZoneClient, *newName, *newValue, *newUnit);
}

# This rule checks that AVU being added, set or removed isn't a protected one.
# Only rodsadmin users are allowed to add, remove or update protected AVUs.
ipc_acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
	if (*Option != 'adda') {
		_ipc_ensureAVUEditable($userNameClient, $rodsZoneClient, *AName, *AValue, *AUnit);
	}
}

# This rule ensures that only the non-protected AVUs are copied from one item to
# another.
ipc_acPreProcForModifyAVUMetadata(
	*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName
) {
	if (!_ipc_isAdmin($userNameClient, $rodsZoneClient)) {
		if (ipc_isCollection(*SourceItemType)) {
			_ipc_cpUnprotectedCollAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
		} else if (ipc_isDataObject(*SourceItemType)) {
			_ipc_cpUnprotectedDataObjAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
		} else if (ipc_isResource(*SourceItemType)) {
			_ipc_cpUnprotectedRescAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
		} else if (ipc_isUser(*SourceItemType)) {
			_ipc_cpUnprotectedUserAVUs(*SourceItemName, *TargetItemType, *TargetItemName);
		}

		# fail to prevent iRODS from also copying the protected metadata
		cut;
		failmsg(0, 'CYVERSE SUCCESS: Successfully copied the unprotected metadata.');
	}
}

# This rule sends a message indicating that an AVU was modified.
ipc_acPostProcForModifyAVUMetadata(
	*Option, *ItemType, *ItemName, *AName, *AValue, *new1, *new2, *new3, *new4
) {
	*newArgs = list(*new1, *new2, *new3, *new4);
	*uuid = '';
	_ipc_ensureUUID(*ItemType, *ItemName, *uuid, $userNameClient, $rodsZoneClient);

	# Determine the original unit and the new AVU settings.
	*origUnit = _ipc_getOrigUnit(*new1);
	*newName = _ipc_getNewAVUSetting(*AName, 'n:', *newArgs);
	*newValue = _ipc_getNewAVUSetting(*AValue, 'v:', *newArgs);
	*newUnit = _ipc_getNewAVUSetting(*origUnit, 'u:', *newArgs);

	# Send AVU modified message.
	_ipc_sendAvuMod(
		*ItemType,
		*uuid,
		*AName,
		*AValue,
		*origUnit,
		*newName,
		*newValue,
		*newUnit,
		$userNameClient,
		$rodsZoneClient );
}

# This rule sends one of the AVU metadata set messages, depending on which
# subcommand was used.
ipc_acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
	if (*AName != _ipc_UUID_ATTR) {
		if (_ipc_contains(*Option, list('add', 'adda', 'rm', 'set'))) {
			*uuid = '';
			_ipc_ensureUUID(*ItemType, *ItemName, *uuid, $userNameClient, $rodsZoneClient);

			_ipc_sendAvuSet(
				*Option, *ItemType, *uuid, *AName, *AValue, *AUnit, $userNameClient, $rodsZoneClient );
		} else if (*Option == 'addw') {
			_ipc_sendAvuMultiset(*ItemName, *AName, *AValue, *AUnit, $userNameClient, $rodsZoneClient);
		} else if (*Option == 'rmw') {
			*uuid = '';
			_ipc_ensureUUID(*ItemType, *ItemName, *uuid, $userNameClient, $rodsZoneClient);

			_ipc_sendAvuMultiremove(
				*ItemType, *uuid, *AName, *AValue, *AUnit, $userNameClient, $rodsZoneClient );
		}
	}
}

# This rules sends an AVU metadata copy message.
ipc_acPostProcForModifyAVUMetadata(
	*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName
) {
	if (!(ipc_isFileSystemType(*TargetItemType) && ipc_inStaging(/*TargetItemName))) {
		*target = _ipc_resolve_msg_entity_id(
			*TargetItemType, *TargetItemName, $userNameClient, $rodsZoneClient );

		if (!(ipc_isFileSystemType(*SourceItemType) && ipc_inStaging(/*SourceItemName))) {
			*source = _ipc_resolve_msg_entity_id(
				*SourceItemType, *SourceItemName, $userNameClient, $rodsZoneClient );

			_ipc_sendAvuCopy(
				*SourceItemType, *source, *TargetItemType, *target, $userNameClient, $rodsZoneClient );
		} else {
			*uuidAttr = _ipc_UUID_ATTR;

			if (ipc_isCollection(*TargetItemType)) {

				foreach( *rec in
					SELECT META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS
					WHERE COLL_NAME == *SourceItemName AND META_COLL_ATTR_NAME != *uuidAttr
				) {
					_ipc_sendAvuSet(
						'add',
						*TargetItemType,
						*target,
						*rec.META_COLL_ATTR_NAME,
						*rec.META_COLL_ATTR_VALUE,
						*rec.META_COLL_ATTR_UNITS,
						$userNameClient,
						$rodsZoneClient );
				}
			} else {
				msiSplitPath(*SourceItemName, *srcCollPath, *srcDataName);

				foreach( *rec in
					SELECT META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS
					WHERE COLL_NAME == *srcCollPath
						AND DATA_NAME == *srcDataName
						AND META_DATA_ATTR_NAME != *uuidAttr
				) {
					_ipc_sendAvuSet(
						'add',
						*TargetItemType,
						*target,
						*rec.META_DATA_ATTR_NAME,
						*rec.META_DATA_ATTR_VALUE,
						*rec.META_DATA_ATTR_UNITS,
						$userNameClient,
						$rodsZoneClient );
				}
			}
		}
	}
}

# Whenever a large file is uploaded, recheck the free space on the storage
# resource server where the file was written.
ipc_acPostProcForParallelTransferReceived(*LeafResource) {
	msi_update_unixfilesystem_resource_free_space(*LeafResource);
}


#
# DYNAMIC PEPS
#

# XXX - Because of https://github.com/irods/irods/issues/5540
# ipc_dataObjCreated_default(*User, *Zone, *DATA_OBJ_INFO) {
# 	*me = 'ipc_dataObjCreated_default';
# 	*id = int(*DATA_OBJ_INFO.data_id);
# 	_ipc_registerAction(*id, *me);
# 	*err = errormsg(_ipc_chksumRepl(*DATA_OBJ_INFO.logical_path, 0), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
# 	*err = errormsg(setAdminGroupPerm(*DATA_OBJ_INFO.logical_path), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
# 	*uuid = ''
# 	_ipc_ensureUUID(
# 		ipc_DATA_OBJECT,
# 		*DATA_OBJ_INFO.logical_path,
# 		*uuid,
# 		*DATA_OBJ_INFO.data_owner_name,
# 		*DATA_OBJ_INFO.data_owner_zone );
#
# 	if (_ipc_isCurrentAction(*id, *me)) {
# 		*err = errormsg(
# 			_ipc_sendDataObjectAdd(
# 				*User,
# 				*Zone,
# 				*uuid,
# 				*DATA_OBJ_INFO.logical_path,
# 				*DATA_OBJ_INFO.data_owner_name,
# 				*DATA_OBJ_INFO.data_owner_zone,
# 				int(*DATA_OBJ_INFO.data_size),
# 				*DATA_OBJ_INFO.data_type ),
# 			*msg );
# 		if (*err < 0) { writeLine('serverLog', *msg); }
# 	}
#
# 	_ipc_unregisterAction(*id, *me);
# }
ipc_dataObjCreated_default(*User, *Zone, *DATA_OBJ_INFO, *Step) {
	*me = 'ipc_dataObjCreated_default';
	*id = int(*DATA_OBJ_INFO.data_id);
	_ipc_registerAction(*id, *me);

	*uuid = '';

	if (*Step != 'FINISH') {
		*err = errormsg(setAdminGroupPerm(*DATA_OBJ_INFO.logical_path), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }
# XXX - Due to a bug in iRODS 4.2.8, msiExecCmd cannot be call from within the
#       pep_database_reg_data_obj_post before the data object's replic has been fully written to
# 		_ipc_ensureUUID(
# 			ipc_DATA_OBJECT,
# 			*DATA_OBJ_INFO.logical_path,
# 			*uuid,
# 			*DATA_OBJ_INFO.data_owner_name,
# 			*DATA_OBJ_INFO.data_owner_zone );
		if (*Step != 'START') {
			_ipc_ensureUUID(
				ipc_DATA_OBJECT,
				*DATA_OBJ_INFO.logical_path,
				*uuid,
				*DATA_OBJ_INFO.data_owner_name,
				*DATA_OBJ_INFO.data_owner_zone );
		}
# XXX - ^^^
	}

	if (*Step != 'START') {
		*err = errormsg(_ipc_chksumRepl(*DATA_OBJ_INFO.logical_path, 0), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }

		if (*uuid == '') {
			_ipc_ensureUUID(
				ipc_DATA_OBJECT,
				*DATA_OBJ_INFO.logical_path,
				*uuid,
				*DATA_OBJ_INFO.data_owner_name,
				*DATA_OBJ_INFO.data_owner_zone );
		}

		if (_ipc_isCurrentAction(*id, *me)) {
			*err = errormsg(
				_ipc_sendDataObjectAdd(
					*User,
					*Zone,
					*uuid,
					*DATA_OBJ_INFO.logical_path,
					*DATA_OBJ_INFO.data_owner_name,
					*DATA_OBJ_INFO.data_owner_zone,
					int(*DATA_OBJ_INFO.data_size),
					*DATA_OBJ_INFO.data_type ),
				*msg );
			if (*err < 0) { writeLine('serverLog', *msg); }

			_ipc_unregisterAction(*id, *me);
		}
	}
}
# XXX - ^^^

# XXX - Because of https://github.com/irods/irods/issues/5540
# ipc_dataObjCreated_staging(*User, *Zone, *DATA_OBJ_INFO) {
# 	*me = 'ipc_dataObjCreated_staging';
# 	*id = int(*DATA_OBJ_INFO.data_id);
# 	_ipc_registerAction(*id, *me);
#
# 	*err = errormsg(_ipc_chksumRepl(*DATA_OBJ_INFO.logical_path, 0), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
# 	*err = errormsg(setAdminGroupPerm(*DATA_OBJ_INFO.logical_path), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
#	_ipc_unregisterAction(*id, *me);
# }
ipc_dataObjCreated_staging(*User, *Zone, *DATA_OBJ_INFO, *Step) {
	*me = 'ipc_dataObjCreated_staging';
	*id = int(*DATA_OBJ_INFO.data_id);
	_ipc_registerAction(*id, *me);

	if (*Step != 'FINISH') {
		*err = errormsg(setAdminGroupPerm(*DATA_OBJ_INFO.logical_path), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }
	}

	if (*Step != 'START') {
		*err = errormsg(_ipc_chksumRepl(*DATA_OBJ_INFO.logical_path, 0), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }
	}

	_ipc_unregisterAction(*id, *me);
}
# XXX - ^^^

ipc_dataObjModified_default(*User, *Zone, *DATA_OBJ_INFO) {
	*me = 'ipc_dataObjModified_default';
	*id = int(*DATA_OBJ_INFO.data_id);
	_ipc_registerAction(*id, *me);

	if (_ipc_isCurrentAction(*id, *me)) {
		*uuid = '';
		_ipc_ensureUUID(
			ipc_DATA_OBJECT,
			*DATA_OBJ_INFO.logical_path,
			*uuid,
			*DATA_OBJ_INFO.data_owner_name,
			*DATA_OBJ_INFO.data_owner_zone );

		_ipc_sendDataObjectMod(
			*User,
			*Zone,
			*uuid,
			*DATA_OBJ_INFO.logical_path,
			*DATA_OBJ_INFO.data_owner_name,
			*DATA_OBJ_INFO.data_owner_zone,
			int(*DATA_OBJ_INFO.data_size),
			*DATA_OBJ_INFO.data_type );

		_ipc_unregisterAction(*id, *me);
	}
}

# This rule sends a system metadata modified status message.
ipc_dataObjMetadataModified(*User, *Zone, *Object) {
	*me = 'ipc_dataObjMetadataModified';
	*id = _ipc_getDataId(*Object);

	if (*id >= 0) {
		_ipc_registerAction(*id, *me);

		if (_ipc_isCurrentAction(*id, *me) && ! ipc_inStaging(/*Object)) {
			*uuid = '';
			_ipc_ensureUUID(ipc_DATA_OBJECT, *Object, *uuid, *User, *Zone);
			_ipc_sendDataObjectMetadataModified(*uuid, *User, *Zone);
			_ipc_unregisterAction(*id, *me);
		}
	}
}
