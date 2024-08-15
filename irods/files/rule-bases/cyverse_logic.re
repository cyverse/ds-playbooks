# The general Data Store rule independent of environment specific rule
# customizations.
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

@include 'cyverse_json'

#
# LISTS
#

_cyverse_logic_contains(*Item, *List) =
	if size(*List) == 0 then false
	else if *Item == hd(*List) then true
	else _cyverse_logic_contains(*Item, tl(*List))


#
# ICAT IDS
#

_cyverse_logic_getCollId(*Path) =
	let *id = -1 in
	let *path = str(*Path) in
	let *_ = foreach (*rec in SELECT COLL_ID WHERE COLL_NAME = *path) { *id = int(*rec.COLL_ID) } in
	*id

_cyverse_logic_getDataObjId(*Path) =
	let *collPath = '' in
	let *dataObjName = '' in
	let *_ = msiSplitPath(str(*Path), *collPath, *dataObjName) in
	let *id = -1 in
	let *_ = foreach ( *rec in
			SELECT DATA_ID WHERE COLL_NAME = *collPath AND DATA_NAME = *dataObjName
		) { *id = int(*rec.DATA_ID) } in
	*id

_cyverse_logic_getId(*Path) =
	if cyverse_isColl(cyverse_getEntityType(*Path)) then _cyverse_logic_getCollId(*Path)
	else _cyverse_logic_getDataObjId(*Path)


#
# USER INFO
#

_cyverse_logic_isAdm(*Username, *UserZone) =
	let *admin = false in
	let *_ = foreach ( *rec in
			SELECT USER_ID
			WHERE USER_NAME = *Username AND USER_ZONE = *UserZone AND USER_TYPE = 'rodsadmin'
		) { *admin = true } in
	*admin


#
# AVUS
#

# Gets the new AVU setting from a list of candidates. New AVU settings are
# passed in an arbitrary order and the type of AVU setting is identified by a
# prefix. This function looks for values matching the given prefix. If multiple
# matching values are found then the last one wins.
#
_cyverse_logic_getNewAVUSetting(*Orig, *Prefix, *Candidates) =
	if size(*Candidates) == 0 then *Orig
	else
		let *candidate = hd(*Candidates) in
		if cyverse_startsWith(*candidate, *Prefix)
		then substr(*candidate, 2, strlen(*candidate))
		else _cyverse_logic_getNewAVUSetting(*Orig, *Prefix, tl(*Candidates))


#
# CHECKSUMS
#

# This compute the checksum of of a given replica of a given data object. This
# rule doesn't fail if the replica to checksum no longer exists.
#
# Parameters:
#  DataObjId  the DB Id of the data object of interest
#  ReplNum    the replica number that will be check summed
#
cyverse_logic_chksumRepl(*DataObjId, *ReplNum) {
	*dataObjPath = '';
	foreach (*rec in
		SELECT COLL_NAME, DATA_NAME WHERE DATA_ID = '*DataObjId' AND DATA_REPL_NUM = '*ReplNum'
	) {
		*dataObjPath = *rec.COLL_NAME ++ '/' ++ *rec.DATA_NAME;
	}

	if (*dataObjPath != '') {
		msiAddKeyValToMspStr('forceChksum', '', *opts);
		msiAddKeyValToMspStr('replNum', str(*ReplNum), *opts);
		msiDataObjChksum(*dataObjPath, *opts, *_);
	}
}

# Schedule a task for computing the checksum of a given replica of a given data object
_cyverse_logic_schedChksumRepl(*DataObjPath, *ReplNum) {
	*dataObjId = _cyverse_logic_getDataObjId(*DataObjPath)

	delay(
		'<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>' ++
		'<PLUSET>0s</PLUSET>' ++
		'<EF>0s REPEAT 0 TIMES</EF>'
	) {cyverse_logic_chksumRepl(*dataObjId, *ReplNum)}
}


#
# UUIDS
#

_cyverse_logic_UUID_ATTR = 'ipc_UUID'

# Assign a UUID to a given collection or data object.
_cyverse_logic_assignUUID(*EntityType, *EntityPath, *Uuid, *ClientName, *ClientZone) {
	*status = 0;

	if (_cyverse_logic_isAdm(*ClientName, *ClientZone)) {
		*path = str(*EntityPath);
		*status = errormsg(
			msiModAVUMetadata(*EntityType, *path, 'set', _cyverse_logic_UUID_ATTR, *Uuid, ''), *msg );

		if (*status != 0) {
			writeLine('serverLog', "DS: Failed to assign UUID to *path");
			writeLine('serverLog', "DS: *msg");
			fail;
		}
	} else {
		cyverse_setProtectedAVU(*EntityPath, _cyverse_logic_UUID_ATTR, *Uuid, '');
	}
}

_cyverse_logic_genUUID() =
	let *uuid = '' in
	let *_ = if (errorcode(msiExecCmd('generateuuid', '', '', '', '', *genResp)) == 0) {
			msiGetStdoutInExecCmdOut(*genResp, *uuid);
			*uuid = trimr(*uuid, "\n");
		} in
	*uuid

# Looks up the UUID of a collection from its path.
_cyverse_logic_getCollUUID(*CollPath) =
	let *coll = str(*CollPath) in
	let *uuid = '' in
	let *attr = _cyverse_logic_UUID_ATTR in
	let *_ = foreach ( *record in
			SELECT META_COLL_ATTR_VALUE WHERE COLL_NAME == *coll AND META_COLL_ATTR_NAME == *attr
		) { *uuid = *record.META_COLL_ATTR_VALUE; } in
	*uuid

# Looks up the UUID of a data object from its path.
_cyverse_logic_getDataObjUUID(*DataObjPath) =
	let *parentColl = '' in
	let *dataName = '' in
	let *_ = msiSplitPath(str(*DataObjPath), *parentColl, *dataName) in
	let *uuid = '' in
	let *attr = _cyverse_logic_UUID_ATTR in
	let *_ = foreach ( *record in
			SELECT META_DATA_ATTR_VALUE
			WHERE COLL_NAME == *parentColl AND DATA_NAME == *dataName AND META_DATA_ATTR_NAME == *attr
		) { *uuid = *record.META_DATA_ATTR_VALUE; } in
	*uuid

# Looks up the UUID for a given type of entity (collection or data object)
_cyverse_logic_getUUID(*EntityType, *EntityPath) =
	if cyverse_isColl(*EntityType) then _cyverse_logic_getCollUUID(*EntityPath)
	else if cyverse_isDataObj(*EntityType) then _cyverse_logic_getDataObjUUID(*EntityPath)
	else ''

_cyverse_logic_ensureUUID(*EntityType, *EntityPath, *ClientName, *ClientZone, *UUID) {
	*uuid = _cyverse_logic_getUUID(*EntityType, *EntityPath);
	if (*uuid == '') {
		*uuid = _cyverse_logic_genUUID();
		if (*uuid == '') {
			writeLine('serverLog', 'Failed to generate UUID for ' ++ str(*EntityPath));
			fail;
		}
		_cyverse_logic_assignUUID(*EntityType, *EntityPath, *uuid, *ClientName, *ClientZone);
	}
	*UUID = *uuid;
}


#
# ACTION TRACKING
#

_cyverse_logic_mkActionKey(*EntityId) = str(*EntityId) ++ '-ROOT_ACTION'

_cyverse_logic_isCurrentAction(*EntityId, *Action) =
	let *key = _cyverse_logic_mkActionKey(*EntityId) in
	if errorcode(temporaryStorage."*key") != 0 then false else temporaryStorage."*key" == *Action

_cyverse_logic_registerAction(*EntityId, *Action) {
	*key = _cyverse_logic_mkActionKey(*EntityId);
	if (if errorcode(temporaryStorage."*key") != 0 then true else temporaryStorage."*key" == '') {
		temporaryStorage."*key" = *Action;
	}
}

_cyverse_logic_unregisterAction(*EntityId, *Action) {
	*key = _cyverse_logic_mkActionKey(*EntityId);
	if (
		if errorcode(temporaryStorage."*key") != 0 then false else temporaryStorage."*key" == *Action
	) {
		temporaryStorage."*key" = '';
	}
}


#
# MESSAGE PUBLISHING
#

_cyverse_logic_COLL_MSG_TYPE = 'collection'
_cyverse_logic_DATA_OBJ_MSG_TYPE = 'data-object'
_cyverse_logic_RESC_MSG_TYPE = 'resource'
_cyverse_logic_USER_MSG_TYPE = 'user'

_cyverse_logic_getMsgType(*EntityType) =
	if cyverse_isColl(*EntityType) then _cyverse_logic_COLL_MSG_TYPE
	else if cyverse_isDataObj(*EntityType) then _cyverse_logic_DATA_OBJ_MSG_TYPE
	else if cyverse_isResc(*EntityType) then _cyverse_logic_RESC_MSG_TYPE
	else if cyverse_isUser(*EntityType) then _cyverse_logic_USER_MSG_TYPE
	else ''

_cyverse_logic_mkAVUObj(*Field, *Attr, *Val, *Unit) =
	cyverse_json_object(
		*Field,
		list(
			cyverse_json_string('attribute', *Attr),
			cyverse_json_string('value', *Val),
			cyverse_json_string('unit', *Unit) ) )

_cyverse_logic_mkUserObj(*Field, *Name, *Zone) = cyverse_json_object(
	*Field, list(cyverse_json_string('name', *Name), cyverse_json_string('zone', *Zone)) )

_cyverse_logic_mkAuthorField(*Name, *Zone) = _cyverse_logic_mkUserObj('author', *Name, *Zone)

_cyverse_logic_mkEntityField(*Uuid) = cyverse_json_string('entity', *Uuid)

_cyverse_logic_mkPathField(*Path) = cyverse_json_string('path', "*Path")

_cyverse_logic_resolveMsgEntityId(*EntityType, *EntityName, *ClientName, *ClientZone, *ID) {
	if (cyverse_isFSType(*EntityType)) {
		_cyverse_logic_ensureUUID(*EntityType, str(*EntityName), *ClientName, *ClientZone, *ID);
	} else {
		*ID = *EntityName;
	}
}

# sends a message to a given AMQP topic exchange
#
# Parameters:
# *Topic (string) the topic of the message
# *Msg (string) the message to send
#
# Remote Execution:
# It executes the amqptopicsend.py command script on the rule engine host
#
_cyverse_logic_sendMsg(*Topic, *Msg) {
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

_cyverse_logic_sendAVUAddWildcard(
	*DataObjPattern, *AttrName, *AttrVal, *AttrUnit, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			cyverse_json_string('pattern', *DataObjPattern),
			_cyverse_logic_mkAVUObj('metadatum', *AttrName, *AttrVal, *AttrUnit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.metadata.addw', *msg);
}

_cyverse_logic_sendAVUCp(*SrcType, *Src, *TgtType, *Tgt, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			cyverse_json_string('source', *Src),
			cyverse_json_string('source-type', _cyverse_logic_getMsgType(*SrcType)),
			cyverse_json_string('destination', *Tgt) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*TgtType) ++ '.metadata.cp', *msg);
}

_cyverse_logic_sendAVUMod(
	*EntityType,
	*Entity,
	*OldName,
	*OldVal,
	*OldUnit,
	*NewName,
	*NewVal,
	*NewUnit,
	*AuthorName,
	*AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Entity),
			_cyverse_logic_mkAVUObj('old-metadatum', *OldName, *OldVal, *OldUnit),
			_cyverse_logic_mkAVUObj('new-metadatum', *NewName, *NewVal, *NewUnit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*EntityType) ++ '.metadata.mod', *msg);
}

_cyverse_logic_sendAVURmWildcard(
	*EntityType, *Entity, *AttrPat, *ValPat, *UnitPat, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Entity),
			cyverse_json_string('attribute-pattern', *AttrPat),
			cyverse_json_string('value-pattern', *ValPat),
			cyverse_json_string('unit-pattern', *UnitPat) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*EntityType) ++ '.metadata.rmw', *msg);
}

_cyverse_logic_sendAVUOp(
	*Op, *EntityType, *Entity, *AttrName, *AttrVal, *AttrUnit, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Entity),
			_cyverse_logic_mkAVUObj('metadatum', *AttrName, *AttrVal, *AttrUnit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*EntityType) ++ '.metadata.' ++ *Op, *msg);
}

_cyverse_logic_sendCollACLMod(
	*Coll, *AccessLvl, *Username, *UserZone, *Recurse, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Coll),
			cyverse_json_boolean('recursive', *Recurse),
			cyverse_json_string('permission', *AccessLvl),
			_cyverse_logic_mkUserObj('user', *Username, *UserZone) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_COLL_MSG_TYPE ++ '.acl.mod', *msg);
}

_cyverse_logic_sendCollInheritMod(*Coll, *Inherit, *Recurse, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Coll),
			cyverse_json_boolean('recursive', *Recurse),
			cyverse_json_boolean('inherit', *Inherit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_COLL_MSG_TYPE ++ '.acl.mod', *msg);
}

_cyverse_logic_sendCollAccessMod(
	*Coll, *AccessLvl, *Username, *UserZone, *Recurse, *AuthorName, *AuthorZone
) {
	if (*AccessLvl == 'inherit') {
		_cyverse_logic_sendCollInheritMod(*Coll, true, *Recurse, *AuthorName, *AuthorZone);
	}
	else if (*AccessLvl == 'noinherit') {
		_cyverse_logic_sendCollInheritMod(*Coll, false, *Recurse, *AuthorName, *AuthorZone);
	}
	else {
		_cyverse_logic_sendCollACLMod(
			*Coll, *AccessLvl, *Username, *UserZone, *Recurse, *AuthorName, *AuthorZone );
	}
}

_cyverse_logic_sendCollAdd(*Id, *Path, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id),
			_cyverse_logic_mkPathField(*Path) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_COLL_MSG_TYPE ++ '.add', *msg);
}

_cyverse_logic_sendDataObjACLMod(
	*DataObj, *AccessLvl, *Username, *UserZone, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*DataObj),
			cyverse_json_string('permission', *AccessLvl),
			_cyverse_logic_mkUserObj('user', *Username, *UserZone) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.acl.mod', *msg);
}

_cyverse_logic_sendDataObjAdd(
	*Id, *Path, *OwnerName, *OwnerZone, *Size, *Type, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id),
			_cyverse_logic_mkPathField(*Path),
			_cyverse_logic_mkUserObj('creator', *OwnerName, *OwnerZone),
			cyverse_json_number('size', *Size),
			cyverse_json_string('type', *Type) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.add', *msg);
}

# Publish a data-object.sys-metadata.mod message to AMQP exchange
_cyverse_logic_sendDataObjMetadataMod(*Id, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.sys-metadata.mod', *msg);
}

# Publish a data-object.mod message to AMQP exchange
_cyverse_logic_sendDataObjMod(
	*Id, *Path, *OwnerName, *OwnerZone, *Size, *Type, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id),
			_cyverse_logic_mkUserObj('creator', *OwnerName, *OwnerZone),
			cyverse_json_number('size', *Size),
			cyverse_json_string('type', *Type) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.mod', *msg);
}

_cyverse_logic_sendDataObjOpen(*Id, *Path, *Size, *AuthorName, *AuthorZone) {
	if (*AuthorName != 'anonymous') {
		msiGetSystemTime(*timestamp, 'human');

		*msg = cyverse_json_document(
			list(
				_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
				_cyverse_logic_mkEntityField(*Id),
				_cyverse_logic_mkPathField(*Path),
				cyverse_json_number('size', *Size),
				cyverse_json_string('timestamp', *timestamp) ) );

		_cyverse_logic_sendMsg(_cyverse_logic_DATA_OBJ_MSG_TYPE ++ '.open', *msg);
	}
}

_cyverse_logic_sendEntityMv(*Type, *Id, *OldPath, *NewPath, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id),
			cyverse_json_string('old-path', '*OldPath'),
			cyverse_json_string('new-path', '*NewPath') ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*Type) ++ '.mv', *msg);
}

_cyverse_logic_sendEntityRm(*Type, *Id, *Path, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Id),
			_cyverse_logic_mkPathField(*Path) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*Type) ++ '.rm', *msg);
}


#
# PROTECTED AVUS
#

# Indicates whether or not an AVU is protected
_cyverse_logic_isAVUProtected(*Attr) = cyverse_startsWith(*Attr, 'ipc')

# Verifies that an attribute can be modified. If it can't it fails and sends an
# error message to the caller.
_cyverse_logic_ensureAVUEditable(*EditorName, *EditorZone, *Attr, *Val, *Unit) {
	if (_cyverse_logic_isAVUProtected(*Attr) && !_cyverse_logic_isAdm(*EditorName, *EditorZone)) {
		cut;
		failmsg(-830000, 'CYVERSE ERROR: attempt to alter protected AVU <*Attr, *Val, *Unit>');
	}
}

# If an AVU is not protected, it sets the AVU to the given item
_cyverse_logic_setAVUIfUnprotected(*EntityType, *EntityName, *Attr, *Val, *Unit) {
	if (!_cyverse_logic_isAVUProtected(*Attr)) {
		msiModAVUMetadata(*EntityType, *EntityName, 'set', *Attr, *Val, *Unit);
	}
}

# Copies the unprotected AVUs from a given collection to the given item.
_cyverse_logic_cpUnprotectedCollAVUs(*CollPath, *TargetType, *TargetName) {
	*path = str(*CollPath);
	foreach (*avu in
		SELECT META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE, META_COLL_ATTR_UNITS
		WHERE COLL_NAME == *path
	) {
		_cyverse_logic_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_COLL_ATTR_NAME,
			*avu.META_COLL_ATTR_VALUE,
			*avu.META_COLL_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given data object to the given item.
_cyverse_logic_cpUnprotectedDataObjAVUs(*DataObjPath, *TargetType, *TargetName) {
	msiSplitPath(str(*DataObjPath), *collPath, *dataObjName);

	foreach (*avu in
		SELECT META_DATA_ATTR_NAME, META_DATA_ATTR_VALUE, META_DATA_ATTR_UNITS
		WHERE COLL_NAME == *collPath AND DATA_NAME == *dataObjName
	) {
		_cyverse_logic_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_DATA_ATTR_NAME,
			*avu.META_DATA_ATTR_VALUE,
			*avu.META_DATA_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given resource to the given item.
_cyverse_logic_cpUnprotectedRescAVUs(*RescName, *TargetType, *TargetName) {
	foreach (*avu in
		SELECT META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
		WHERE RESC_NAME == *RescName
	) {
		_cyverse_logic_setAVUIfUnprotected(
			*TargetType,
			*TargetName,
			*avu.META_RESC_ATTR_NAME,
			*avu.META_RESC_ATTR_VALUE,
			*avu.META_RESC_ATTR_UNITS );
	}
}

# Copies the unprotected AVUs from a given user to the given item.
_cyverse_logic_cpUnprotectedUserAVUs(*Username, *TargetType, *TargetName) {
	*nameParts = split(*Username, '#');
	*query =
		if size(*nameParts) == 2 then
		   # A zone was provided.
			let *name = elem(*nameParts, 0) in
			let *zone = elem(*nameParts, 1) in
			SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
			WHERE USER_NAME == *name AND USER_ZONE == *zone
		else
			SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
			WHERE USER_NAME == *Username;
	foreach (*avu in *query) {
		_cyverse_logic_setAVUIfUnprotected(
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

_cyverse_logic_resolveAdmPerm(*Path) =
	if str(*Path) like regex '/' ++ cyverse_ZONE ++ '(/trash)?/home/.*' then 'own' else 'write'

_cyverse_logic_setAdmPerm(*Path) {
	msiSetACL('default', _cyverse_logic_resolveAdmPerm(*Path), 'rodsadmin', str(*Path));
}


#
# STATIC PEPS
#

# This rule prevents the user from removing rodsadmin's ownership from an ACL
# unless the user is of type rodsadmin.
#
# Parameters:
#  RecurseFlag  (unused)
#  Perm         (string) the permission being granted to *Username, if the value
#               is "null", "read", "write", or "own", enable inheritance if the
#               value is "inherit", or disable inheritance if the value is
#               "noinherit". If the value is prefixed with "admin:", this is an
#               administrative ACL change.
#  Username     (string) the account or group being given *Perm, ignored if
#               *Perm is "inherit" or "noinherit"
#  Zone         (unused)
#  Path         (string) the path to the collection or data object whose ACL is
#               being altered
#
cyverse_logic_acPreProcForModifyAccessControl(*RecurseFlag, *Perm, *Username, *Zone, *Path) {
	if (*Username == 'rodsadmin') {
		if (!(*Perm like 'admin:*') && *Perm != _cyverse_logic_resolveAdmPerm(*Path)) {
			cut;
			failmsg(-830000, 'CYVERSE ERROR: attempt to alter admin user permission.');
		}
	}
}

# This sends a collection or data-object ACL modification message for the
# updated object.
#
# Parameters:
#  RecurseFlag  (string) indicates if the permission change applied recursively
#               to the contents of a *Path, "1" indicates the flag was present,
#               and "0" indicates the opposite.
#  Perm         (string) the permission granted to *Username, if the value was
#               "null", "read", "write", or "own", enabled inheritance if the
#               value was "inherit", or disabled inheritance if the value was
#               "noinherit". If the value is prefixed with "admin:", this is an
#               administrative ACL change.
#  Username     (string) the account or group given *Perm, ignored if *Perm was
#               "inherit" or "noinherit"
#  UserZone     (string) the zone where *UserName belongs, ignored if *Perm is
#               "inherit" or "noinherit"
#  Path         (string) the path to the collection or data object whose ACL was
#               altered
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPostProcForModifyAccessControl(*RecurseFlag, *Perm, *Username, *UserZone, *Path) {
	*me = 'ipc_acPostProcForModifyAccessControl';
	*entityId = _cyverse_logic_getId(*Path);

	if (*entityId >= 0) {
		_cyverse_logic_registerAction(*entityId, *me);

		if (_cyverse_logic_isCurrentAction(*entityId, *me)) {
			*lvl = cyverse_rmPrefix(*Perm, list('admin:'));
			*type = cyverse_getEntityType(*Path);
			*userZone = if *UserZone == '' then cyverse_ZONE else *UserZone;
			*uuid = '';
			_cyverse_logic_ensureUUID(*type, *Path, $userNameClient, $rodsZoneClient, *uuid);

			if (cyverse_isColl(*type)) {
				_cyverse_logic_sendCollAccessMod(
					*uuid,
					*lvl,
					*Username,
					*userZone,
					bool(*RecurseFlag),
					$userNameClient,
					$rodsZoneClient );
			} else if (cyverse_isDataObj(*type)) {
				_cyverse_logic_sendDataObjACLMod(
					*uuid, *lvl, *Username, *userZone, $userNameClient, $rodsZoneClient );
			}

			_cyverse_logic_unregisterAction(*entityId, *me);
		}
	}
}

# This rule checks that AVU being added, set or removed isn't a protected one.
# Only rodsadmin users are allowed to add, remove or update protected AVUs.
#
# Parameters:
#  Opt         (string) the subCommand, one of 'add', 'adda', 'addw', 'rm',
#              'rmi', 'rmw', or 'set'
#  EntityType  (unused)
#  EntityName  (unused)
#  Attr        (string) the name of the attribute
#  Val         (string) the value of the attribute
#  Unit        (string) the unit of the attribute
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPreProcForModifyAVUMetadata(*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit) {
	if (*Opt != 'adda') {
		_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *Attr, *Val, *Unit);
	}
}

# This rule checks that AVU being modified isn't a protected one.
#
# Parameters:
#  Opt         (unused)
#  EntityType  (unused)
#  EntityName  (unused)
#  Attr        (string) the attribute name before modification
#  Val         (string) the attribute value before modification
#  Unit        (string) the attribute unit before modification
#  New1        (string) holds an update to the name, value, or unit prefixed by
#              'n:', 'v:', or 'u:', respectively
#  New2        (string) either empty or holds an update to the name, value, or
#              unit prefixed by 'n:', 'v:', or 'u:', respectively
#  New3        (string) either empty or holds an update to the name, value, or
#              unit prefixed by 'n:', 'v:', or 'u:', respectively
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
# XXX: Due to a bug in iRODS 4.2.8, when a unitless AVU is modified to have a new attribute name,
#      value, and unit in a single call, the new unit is not passed in.
#
cyverse_logic_acPreProcForModifyAVUMetadata(
	*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit, *New1, *New2, *New3
) {
	*newArgs = list(*New1, *New2, *New3);

	# Determine the new AVU settings.
	*newAttr = _cyverse_logic_getNewAVUSetting(*Attr, 'n:', *newArgs);
	*newVal = _cyverse_logic_getNewAVUSetting(*Val, 'v:', *newArgs);
	*newUnit = _cyverse_logic_getNewAVUSetting(*Unit, 'u:', *newArgs);

	_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *Attr, *Val, *Unit);
	_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *newAttr, *newVal, *newUnit);
}

# This rule ensures that only the non-protected AVUs are copied from one item to
# another.
#
# Parameters:
#  Opt      (unused)
#  SrcType  (string) the type of entity whose AVUs are being copied, '-C' for
#           collection, '-d' for data object, '-R' for resource, or '-u' for
#           user
#  TgtType  (string) the type of entity receiving the AVUs, '-C' for collection,
#           '-d' for data object, '-R' for resource, or '-u' for user
#  SrcName  (string) the name of the entity whose AVUs are being copied, for a
#           collection or data object, this is the entity's absolute path
#  TgtName  (string) the name of the entity receiving the AVUs, for a collection
#           or data object, this is the entity's absolute path
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPreProcForModifyAVUMetadata(*Opt, *SrcType, *TgtType, *SrcName, *TgtName) {
	if (!_cyverse_logic_isAdm($userNameClient, $rodsZoneClient)) {
		if (cyverse_isColl(*SrcType)) {
			_cyverse_logic_cpUnprotectedCollAVUs(*SrcName, *TgtType, *TgtName);
		} else if (cyverse_isDataObj(*SrcType)) {
			_cyverse_logic_cpUnprotectedDataObjAVUs(*SrcName, *TgtType, *TgtName);
		} else if (cyverse_isResc(*SrcType)) {
			_cyverse_logic_cpUnprotectedRescAVUs(*SrcName, *TgtType, *TgtName);
		} else if (cyverse_isUser(*SrcType)) {
			_cyverse_logic_cpUnprotectedUserAVUs(*SrcName, *TgtType, *TgtName);
		}

		# fail to prevent iRODS from also copying the protected metadata
		cut;
		failmsg(0, 'CYVERSE SUCCESS: Successfully copied the unprotected metadata.');
	}
}

# This rule sends one of the AVU metadata set messages, depending on which
# subcommand was used.
#
# Parameters:
#  Opt         (string) the subCommand, one of 'add', 'adda', 'addw', 'rm',
#              'rmw', 'rmi', or 'set'
#  EntityType  (string) the type of entity whose AVU was modified, '-C' for
#              collection, '-d' for data object, '-R' for resource, or '-u' for
#              user
#  EntityName  (string) the name of the entity whose AVU was modified, this is
#              an absolute path for a collection or data object
#  Attr        (string) the name of the attribute
#  Val         (string) the value of the attribute
#  Unit        (string) the unit of the attribute
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPostProcForModifyAVUMetadata(*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit) {
	if (cyverse_isFSType(*EntityType) && *Attr != _cyverse_logic_UUID_ATTR) {
		if (_cyverse_logic_contains(*Opt, list('add', 'adda', 'rm', 'set'))) {
			*uuid = '';

			_cyverse_logic_ensureUUID(
				*EntityType, *EntityName, $userNameClient, $rodsZoneClient, *uuid );

			_cyverse_logic_sendAVUOp(
				*Opt, *EntityType, *uuid, *Attr, *Val, *Unit, $userNameClient, $rodsZoneClient );
		} else if (*Opt == 'addw') {
			_cyverse_logic_sendAVUAddWildcard(
				*EntityName, *Attr, *Val, *Unit, $userNameClient, $rodsZoneClient );
		} else if (*Opt == 'rmw') {
			*uuid = '';

			_cyverse_logic_ensureUUID(
				*EntityType, *EntityName, $userNameClient, $rodsZoneClient, *uuid );

			_cyverse_logic_sendAVURmWildcard(
				*EntityType, *uuid, *Attr, *Val, *Unit, $userNameClient, $rodsZoneClient );
		}
	}
}

# This rule sends a message indicating that an AVU was modified.
#
# Parameters:
#  Opt         (unused)
#  EntityType  (string) the type of entity whose AVU was modified, '-C' for
#              collection, '-d' for data object, '-R' for resource, or '-u' for
#              user
#  EntityName  (string) the name of the entity whose AVU was modified, this is
#              an absolute path for a collection or data object
#  Attr        (string) the attribute name before modification
#  Val         (string) the attribute value before modification
#  Unit        (string) the attribute unit before modification
#  New1        (string) holds the updated name, value, or unit prefixed by 'n:',
#              'v:', or 'u:', respectively
#  New2        (string) either empty or holds the updated name, value, or unit
#              prefixed by 'n:', 'v:', or 'u:', respectively
#  New3        (string) either empty or holds the updated name, value, or unit
#              prefixed by 'n:', 'v:', or 'u:', respectively
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
# XXX: Due to a bug in iRODS 4.2.8, when a unitless AVU is modified to have a new attribute name,
#      value, and unit in a single call, the new unit is not passed in.
#
cyverse_logic_acPostProcForModifyAVUMetadata(
	*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit, *New1, *New2, *New3
) {
	*newArgs = list(*New1, *New2, *New3);
	*uuid = '';
	_cyverse_logic_ensureUUID(*EntityType, *EntityName, $userNameClient, $rodsZoneClient, *uuid);

	# Determine the new AVU settings.
	*newAttr = _cyverse_logic_getNewAVUSetting(*Attr, 'n:', *newArgs);
	*newVal = _cyverse_logic_getNewAVUSetting(*Val, 'v:', *newArgs);
	*newUnit = _cyverse_logic_getNewAVUSetting(*Unit, 'u:', *newArgs);

	# Send AVU modified message.
	_cyverse_logic_sendAVUMod(
		*EntityType,
		*uuid,
		*Attr,
		*Val,
		*Unit,
		*newAttr,
		*newVal,
		*newUnit,
		$userNameClient,
		$rodsZoneClient );
}

# This rules sends an AVU metadata copy message.
#
# Parameters:
#  Opt      (unused)
#  SrcType  (string) the type of entity whose AVUs were copied, '-C' for
#           collection, '-d' for data object, '-R' for resource, or '-u' for
#           user
#  TgtType  (string) the type of entity that received the AVUs, '-C' for
#           collection, '-d' for data object, '-R' for resource, or '-u' for
#           user
#  SrcName  (string) the name of the entity whose AVUs were copied, for a
#           collection or data object, this is the entity's absolute path
#  TgtName  (string) the name of the entity that received the AVUs, for a
#           collection or data object, this is the entity's absolute path
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPostProcForModifyAVUMetadata(*Opt, *SrcType, *TgtType, *SrcName, *TgtName) {
	if (cyverse_isFSType(*TgtType) && !cyverse_inStaging(/*TgtName)) {
		*tgt = '';
		_cyverse_logic_resolveMsgEntityId(*TgtType, *TgtName, $userNameClient, $rodsZoneClient, *tgt);

		if (cyverse_isFSType(*SrcType) && !cyverse_inStaging(/*SrcName)) {
			*src = '';
			_cyverse_logic_resolveMsgEntityId(
				*SrcType, *SrcName, $userNameClient, $rodsZoneClient, *src );

			_cyverse_logic_sendAVUCp(*SrcType, *src, *TgtType, *tgt, $userNameClient, $rodsZoneClient);
		} else {
			*uuidAttr = _cyverse_logic_UUID_ATTR;

			if (cyverse_isResc(*SrcType)) {
				foreach( *rec in
					SELECT META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
					WHERE RESC_NAME == *SrcName AND META_RESC_ATTR_NAME != *uuidAttr
				) {
					_cyverse_logic_sendAVUOp(
						'add',
						*TgtType,
						*tgt,
						*rec.META_RESC_ATTR_NAME,
						*rec.META_RESC_ATTR_VALUE,
						*rec.META_RESC_ATTR_UNITS,
						$userNameClient,
						$rodsZoneClient );
				}
			} else {
				foreach( *rec in
					SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
					WHERE USER_NAME == *SrcName AND META_USER_ATTR_NAME != *uuidAttr
				) {
					_cyverse_logic_sendAVUOp(
						'add',
						*TgtType,
						*tgt,
						*rec.META_USER_ATTR_NAME,
						*rec.META_USER_ATTR_VALUE,
						*rec.META_USER_ATTR_UNITS,
						$userNameClient,
						$rodsZoneClient );
				}
			}
		}
	}
}

# This rule sets the rodsadmin group permission of a collection when a
# collection is created by an administrative means, i.e. iadmin mkuser. It also
# assigns a UUID and pushes a collection.add message into the irods exchange.
#
# Parameters:
#  ParCollPath  (string) the absolute path to the parent of the collection being
#               created
#  CollName     (string) the name of the collection being created
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acCreateCollByAdmin(*ParCollPath, *CollName) {
	*coll = *ParCollPath ++ '/' ++ *CollName;
	*perm = _cyverse_logic_resolveAdmPerm(*coll);

	*err = errormsg(msiSetACL('default', 'admin:*perm', 'rodsadmin', *coll), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}

	*uuid = '';
	_cyverse_logic_ensureUUID(cyverse_COLL, *coll, $userNameClient, $rodsZoneClient, *uuid);
	_cyverse_logic_sendCollAdd(*uuid, *coll, $userNameClient, $rodsZoneClient);
}

# This rule makes the admin owner of any created collection. It also assigns a
# UUID and publishes a collection.add message to the irods exchange. This rule
# is not applied to collections created when a TAR file is expanded. (i.e.
# ibun -x)
#
cyverse_logic_acPostProcForCollCreate {
	*err = errormsg(_cyverse_logic_setAdmPerm($collName), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}

	*uuid = '';
	_cyverse_logic_ensureUUID(cyverse_COLL, $collName, $userNameClient, $rodsZoneClient, *uuid);
	_cyverse_logic_sendCollAdd(*uuid, $collName, $userNameClient, $rodsZoneClient);
}

# This rule pushes a collection.rm message into the irods exchange.
#
# Parameters:
#  ParCollPath  (string) the absolute path to the parent collection of the
#               collection being deleted
#  CollName     (string) the name of collection being deleted
#
# Session Variables:
#  rodsZoneClient
#  userNameClient
#
cyverse_logic_acDeleteCollByAdmin(*ParCollPath, *CollName) {
	*path = *ParCollPath ++ '/' ++ *CollName;
	*uuid = _cyverse_logic_getCollUUID(*path);

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(cyverse_COLL, *uuid, *path, $userNameClient, $rodsZoneClient);
	}
}

# This rule pushes a collection.rm message into the irods exchange.
#
# Parameters:
#  ParCollPath  (string) the absolute path to the parent collection of the
#               collection being deleted
#  CollName     (string) the name of collection being deleted
#
# Session Variables:
#  rodsZoneClient
#  userNameClient
#
cyverse_logic_acDeleteCollByAdminIfPresent(*ParCollPath, *CollName) {
	cyverse_logic_acDeleteCollByAdmin(*ParCollPath, *CollName);
}

# This rule stores the name UUID of a collection that is about to be deleted for
# use by cyverse_logic_acPostProcForRmColl.
#
# Session Variables:
#  collName
#
# temporaryStorage:
#  '$collName'  writes the UUID here
#
cyverse_logic_acPreprocForRmColl {
	temporaryStorage.'$collName' = _cyverse_logic_getCollUUID($collName);
}

# This rule publishes a removal message for a collection.
#
# Session Variables:
#  collName
#  userNameClient
#  rodsZoneClient
#
# temporaryStorage:
#  '$collName'  reads the UUID from here
#
cyverse_logic_acPostProcForRmColl {
	*uuid = temporaryStorage.'$collName';

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(cyverse_COLL, *uuid, $collName, $userNameClient, $rodsZoneClient);
	}
}

# Refuse SSL connections
#
# Parameters:
#  OUT  (string) "CS_NEG_REFUSE"
#
cyverse_logic_acPreConnect(*OUT) {
	*OUT = 'CS_NEG_REFUSE';
}

# This rule ensures that the storage resource free space is updated when a
# data object is replicated to it.
#
# Parameters:
#  StoreResc  (string) the name of the storage resource where the replica was
#             stored
#
cyverse_logic_acPostProcForDataCopyReceived(*StoreResc) {
	msi_update_unixfilesystem_resource_free_space(*StoreResc);
}

# This rule stores the UUID of a data object that is about to be deleted for use
# by cyverse_logic_acPostProcForDelete.
#
# Session Variables:
#  objPath
#
# temporaryStorage:
#  '$objPath'  writes the UUID here
#
cyverse_logic_acDataDeletePolicy {
	temporaryStorage.'$objPath' = _cyverse_logic_getDataObjUUID($objPath);
}

# This rule publishes a removal message for a data object.
#
# Session Variables:
#  objPath
#  userNameClient
#  rodsZoneClient
#
# temporaryStorage:
#  '$objPath' reads the UUID from here
#
cyverse_logic_acPostProcForDelete {
	*uuid = temporaryStorage.'$objPath';

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(
			cyverse_DATA_OBJ, *uuid, $objPath, $userNameClient, $rodsZoneClient );
	}
}

# This rule publishes a data object access message.
#
# Session Variables:
#  objPath
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPostProcForOpen {
	*me = 'ipc_acPostProcForOpen';
	*id = _cyverse_logic_getDataObjId($objPath);

	if (*id >= 0) {
		_cyverse_logic_registerAction(*id, *me);

		if (_cyverse_logic_isCurrentAction(*id, *me)) {
			*uuid = '';

			_cyverse_logic_ensureUUID(
				cyverse_DATA_OBJ, $objPath, $userNameClient, $rodsZoneClient, *uuid );

			_cyverse_logic_sendDataObjOpen(
				*uuid, $objPath, $dataSize, $userNameClient, $rodsZoneClient );

			_cyverse_logic_unregisterAction(*id, *me);
		}
	}
}

# This rule schedules a rename entry job for the data object or collection being
# renamed.
#
# Parameters:
#  SrcEntity   (string) the path to the collection or data object prior to being
#              moved or renamed.
#  DestEntity  (string) the new path
#
# Session Variables:
#  userNameClient
#  rodsZoneClient
#
cyverse_logic_acPostProcForObjRename(*SrcEntity, *DestEntity) {
	*type = cyverse_getEntityType(*DestEntity);
	*uuid = '';
	_cyverse_logic_ensureUUID(*type, *DestEntity, $userNameClient, $rodsZoneClient, *uuid);

	if (*uuid != '') {
		_cyverse_logic_sendEntityMv(
			*type, *uuid, *SrcEntity, *DestEntity, $userNameClient, $rodsZoneClient );
	}
}

# Use default threading setting
#
# Session Variables:
#  dataSize
#  numThreads
#  windowSize
#
cyverse_logic_acSetNumThreads {
	msiSetNumThreads('default', 'default', 'default');
}

# Whenever a large file is uploaded, recheck the free space on the storage
# resource server where the file was written.
#
# Parameters:
#  StoreResc  (string) the name of the storage resource where the replica was
#             stored
#
cyverse_logic_acPostProcForParallelTransferReceived(*StoreResc) {
	msi_update_unixfilesystem_resource_free_space(*StoreResc);
}

# Set maximum number of rule engine processes
#
cyverse_logic_acSetReServerNumProc {
	msiSetReServerNumProc(str(cyverse_MAX_NUM_RE_PROCS));
}

# Create a user for a Data Store service
#
# Session Variables:
#  authFlag
#
cyverse_logic_acCreateUser {
	msiCreateUser ::: msiRollback;
	msiCommit;
}


#
# DYNAMIC PEPS
#

# This gives rodsadmin own permission, generates a UUID, publishes a data object
# create message and schedules a checksum computation for a data object.
#
# Parameters:
#  Username     (string) the iRODS account that created the data object
#  Zone         (string) the zone of the username who created the data object
#  DataObjInfo  (`KeyValuePair_PI`) the DATA_OBJ_INFO map for a data object
#               metadata modification or registration event
# XXX - Because of https://github.com/irods/irods/issues/5540
# #
# cyverse_logic_dataObjCreated(*Username, *Zone, *DataObjInfo) {
# 	*me = 'cyverse_logic_dataObjCreated';
# 	*id = int(*DataObjInfo.data_id);
# 	_cyverse_logic_registerAction(*id, *me);
# 	*err = errormsg(_cyverse_logic_schedChksumRepl(*DataObjInfo.logical_path, 0), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
# 	*err = errormsg(_cyverse_logic_setAdmPerm(*DataObjInfo.logical_path), *msg);
# 	if (*err < 0) { writeLine('serverLog', *msg); }
#
# 	*uuid = ''
# 	_cyverse_logic_ensureUUID(
# 		cyverse_DATA_OBJ,
# 		*DataObjInfo.logical_path,
# 		*DataObjInfo.data_owner_name,
# 		*DataObjInfo.data_owner_zone,
# 		*uuid );
#
# 	if (_cyverse_logic_isCurrentAction(*id, *me)) {
# 		*err = errormsg(
# 			_cyverse_logic_sendDataObjAdd(
# 				*uuid,
# 				*DataObjInfo.logical_path,
# 				*DataObjInfo.data_owner_name,
# 				*DataObjInfo.data_owner_zone,
# 				int(*DataObjInfo.data_size),
# 				*DataObjInfo.data_type,
# 				*Username,
# 				*Zone ),
# 			*msg );
# 		if (*err < 0) { writeLine('serverLog', *msg); }
# 	}
#
# 	_cyverse_logic_unregisterAction(*id, *me);
# }
#  Step  (string) the current step in the transfer process, one of 'FULL',
#        'START', or 'FINISH'
#
cyverse_logic_dataObjCreated(*Username, *Zone, *DataObjInfo, *Step) {
	*me = 'cyverse_logic_dataObjCreated';
	*id = int(*DataObjInfo.data_id);
	_cyverse_logic_registerAction(*id, *me);

	*uuid = '';

	if (*Step != 'FINISH') {
		*err = errormsg(_cyverse_logic_setAdmPerm(*DataObjInfo.logical_path), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }
# XXX - Due to a bug in iRODS 4.2.8, msiExecCmd cannot be call from within the
#       pep_database_reg_data_obj_post before the data object's replica has been
#       fully written to storage. This means that the _cyverse_logic_ensureUUID
#       cannot be called when *Step is START.
# 		_cyverse_logic_ensureUUID(
# 			cyverse_DATA_OBJ,
# 			*DataObjInfo.logical_path,
# 			*DataObjInfo.data_owner_name,
# 			*DataObjInfo.data_owner_zone,
# 			*uuid );
		if (*Step != 'START') {
			_cyverse_logic_ensureUUID(
				cyverse_DATA_OBJ,
				*DataObjInfo.logical_path,
				*DataObjInfo.data_owner_name,
				*DataObjInfo.data_owner_zone,
				*uuid );
		}
# XXX - ^^^
	}

	if (*Step != 'START') {
		*err = errormsg(_cyverse_logic_schedChksumRepl(*DataObjInfo.logical_path, 0), *msg);
		if (*err < 0) { writeLine('serverLog', *msg); }

		if (*uuid == '') {
			_cyverse_logic_ensureUUID(
				cyverse_DATA_OBJ,
				*DataObjInfo.logical_path,
				*DataObjInfo.data_owner_name,
				*DataObjInfo.data_owner_zone,
				*uuid );
		}

		if (_cyverse_logic_isCurrentAction(*id, *me)) {
			*err = errormsg(
				_cyverse_logic_sendDataObjAdd(
					*uuid,
					*DataObjInfo.logical_path,
					*DataObjInfo.data_owner_name,
					*DataObjInfo.data_owner_zone,
					int(*DataObjInfo.data_size),
					*DataObjInfo.data_type,
					*Username,
					*Zone ),
				*msg );
			if (*err < 0) { writeLine('serverLog', *msg); }

			_cyverse_logic_unregisterAction(*id, *me);
		}
	}
}
# XXX - ^^^

# This ensures that a data object has a UUID and publishes a data object
# modification message.
#
# Parameters:
#  Username     (string) the iRODS account that modified the data object
#  Zone         (string) the zone of the username who modified the data object
#  DataObjInfo  (`KeyValuePair_PI`) the DATA_OBJ_INFO map for a data object
#               modification event
#
cyverse_logic_dataObjMod(*Username, *Zone, *DataObjInfo) {
	*me = 'cyverse_logic_dataObjMod';
	*id = int(*DataObjInfo.data_id);
	_cyverse_logic_registerAction(*id, *me);

	if (_cyverse_logic_isCurrentAction(*id, *me)) {
		*uuid = '';
		_cyverse_logic_ensureUUID(
			cyverse_DATA_OBJ,
			*DataObjInfo.logical_path,
			*DataObjInfo.data_owner_name,
			*DataObjInfo.data_owner_zone,
			*uuid );

		_cyverse_logic_sendDataObjMod(
			*uuid,
			*DataObjInfo.logical_path,
			*DataObjInfo.data_owner_name,
			*DataObjInfo.data_owner_zone,
			int(*DataObjInfo.data_size),
			*DataObjInfo.data_type,
			*Username,
			*Zone );

		_cyverse_logic_unregisterAction(*id, *me);
	}
}

# This ensures that a data object has a UUID and publishes a data object
# system metadata modification message.
#
# Parameters:
#  Username  (string) the iRODS account that modified the data object
#  Zone      (string) the zone of the username who modified the data object
#  Path      (path) the path to the modified data object
#
cyverse_logic_dataObjMetaMod(*Username, *Zone, *Path) {
	*me = 'cyverse_logic_dataObjMetadataMod';
	*id = _cyverse_logic_getDataObjId(*Path);

	if (*id >= 0) {
		_cyverse_logic_registerAction(*id, *me);

		if (_cyverse_logic_isCurrentAction(*id, *me)) {
			*uuid = '';
			_cyverse_logic_ensureUUID(cyverse_DATA_OBJ, *Path, *Username, *Zone, *uuid);
			_cyverse_logic_sendDataObjMetadataMod(*uuid, *Username, *Zone);
			_cyverse_logic_unregisterAction(*id, *me);
		}
	}
}
