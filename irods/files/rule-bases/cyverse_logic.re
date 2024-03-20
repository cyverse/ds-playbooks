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
# STRINGS
#

# Determines whether or not the string in the first argument starts with the
# string in the second argument.
_cyverse_logic_startsWith(*Str, *Prefix) =
	if strlen(*Str) < strlen(*Prefix) then false
	else if substr(*Str, 0, strlen(*Prefix)) != *Prefix then false
	else true

# Removes a prefix from a string.
_cyverse_logic_rmPrefix(*Orig, *Prefixes) =
	if size(*Prefixes) == 0 then *Orig
	else
		if _cyverse_logic_startsWith(*Orig, hd(*Prefixes))
		then substr(*Orig, strlen(hd(*Prefixes)), strlen(*Orig))
		else _cyverse_logic_rmPrefix(*Orig, tl(*Prefixes))


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
		if _cyverse_logic_startsWith(*candidate, *Prefix)
		then substr(*candidate, 2, strlen(*candidate))
		else _cyverse_logic_getNewAVUSetting(*Orig, *Prefix, tl(*Candidates))

# Gets the original unit for an AVU modification. The argument that is used for
# the original unit in the AVU modification may contain the original unit or, if
# the unit was empty in the original AVU then this argument may contain the
# first of the new AVU settings instead. We can distinguish this case from the
# others by the presence of a prefix in the value. The prefix is always a single
# character followed by a colon.
#
_cyverse_logic_getOrigUnit(*Candidate) =
	if strlen(*Candidate) < 2 then *Candidate
	else if substr(*Candidate, 1, 1) != ':' then *Candidate
	else ''


#
# CHECKSUMS
#

# Compute the checksum of of a given replica of a given data object
_cyverse_logic_chksumRepl(*DataObjId, *ReplNum) {
	*dataObjPath = '';
	foreach (*rec in SELECT COLL_NAME, DATA_NAME WHERE DATA_ID = '*DataObjId') {
		*dataObjPath = *rec.COLL_NAME ++ '/' ++ *rec.DATA_NAME;
	}

	if (*dataObjPath != '') {
		*opts = '';
		msiAddKeyValToMspStr('forceChksum', '', *opts);
		msiAddKeyValToMspStr('replNum', str(*ReplNum), *opts);
		msiDataObjChksum(*dataObjPath, *opts, *_);
	}
}

# Schedule a task for computing the checksum of a given replica of a given data object
_cyverse_logic_schedChksumRepl(*DataObjPath, *ReplNum) {
	*dataObjId = _ipc_getDataId(*DataObjPath)

	delay(
		'<INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>' ++
		'<PLUSET>0s</PLUSET>' ++
		'<EF>0s REPEAT 0 TIMES</EF>'
	) {_cyverse_logic_chksumRepl(*dataObjId, *ReplNum)}
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
	let *genResp = '' in
	let *status = errorcode(msiExecCmd("generateuuid", "", "null", "null", "null", *genResp)) in
	let *_ = if (*status == 0) {
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
	let *key = _cyverse_logic_mkActionKey(*EntityId) in temporaryStorage."*key" == *Action

_cyverse_logic_registerAction(*EntityId, *Action) {
	*key = _cyverse_logic_mkActionKey(*EntityId);
	if (if errorcode(temporaryStorage."*key") != 0 then true else temporaryStorage."*key" == '') {
		temporaryStorage."*key" = *Action;
	}
}

_cyverse_logic_unregisterAction(*EntityId, *Action) {
	*key = _cyverse_logic_mkActionKey(*EntityId);
	if (temporaryStorage."*key" == *Action) {
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

_cyverse_logic_resolveMsgEntityId(*EntityType, *EntityName, *ClientName, *ClientZone) =
	let *id = '' in
	let *_ =
		if cyverse_isFSType(*EntityType)
		then _cyverse_logic_ensureUUID(*EntityType, str(*EntityName), *ClientName, *ClientZone, *id)
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

_cyverse_logic_sendAVUCp(*SourceType, *Source, *TargetType, *Target, *AuthorName, *AuthorZone) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			cyverse_json_string('source', *Source),
			cyverse_json_string('source-type', _cyverse_logic_getMsgType(*SourceType)),
			cyverse_json_string('destination', *Target) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*TargetType) ++ '.metadata.cp', *msg);
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
	*EntityType, *Entity, *AttrName, *AttrVal, *AttrUnit, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Entity),
			cyverse_json_string('attribute-pattern', *AttrName),
			cyverse_json_string('value-pattern', *AttrVal),
			cyverse_json_string('unit-pattern', *AttrUnit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*EntityType) ++ '.metadata.rmw', *msg);
}

_cyverse_logic_sendAVUSet(
	*Opt, *EntityType, *Entity, *AttrName, *AttrVal, *AttrUnit, *AuthorName, *AuthorZone
) {
	*msg = cyverse_json_document(
		list(
			_cyverse_logic_mkAuthorField(*AuthorName, *AuthorZone),
			_cyverse_logic_mkEntityField(*Entity),
			_cyverse_logic_mkAVUObj('metadatum', *AttrName, *AttrVal, *AttrUnit) ) );

	_cyverse_logic_sendMsg(_cyverse_logic_getMsgType(*EntityType) ++ '.metadata.' ++ *Opt, *msg);
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
_cyverse_logic_isAVUProtected(*Attr) = _cyverse_logic_startsWith(*Attr, 'ipc')

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
	foreach (*avu in
		SELECT META_USER_ATTR_NAME, META_USER_ATTR_VALUE, META_USER_ATTR_UNITS
		WHERE USER_NAME == *Username
	) {
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
cyverse_logic_acPostProcForModifyAccessControl(*RecurseFlag, *Perm, *Username, *UserZone, *Path) {
	*me = 'ipc_acPostProcForModifyAccessControl';
	*entityId = _cyverse_logic_getId(*Path);

	if (*entityId >= 0) {
		_cyverse_logic_registerAction(*entityId, *me);

		if (_cyverse_logic_isCurrentAction(*entityId, *me)) {
			*lvl = _cyverse_logic_rmPrefix(*Perm, list('admin:'));
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

# This rule checks that AVU being modified isn't a protected one.
cyverse_logic_acPreProcForModifyAVUMetadata(
	*Opt, *EntityType, *EntityName, *Attr, *Val, *UnitOrNew1, *New2, *New3, *New4
) {
	*newArgs = list(*UnitOrNew1, *New2, *New3, *New4);

	# Determine the original unit and the new AVU settings.
	*origUnit = _cyverse_logic_getOrigUnit(*UnitOrNew1);
	*newAttr = _cyverse_logic_getNewAVUSetting(*Attr, 'n:', *newArgs);
	*newVal = _cyverse_logic_getNewAVUSetting(*Val, 'v:', *newArgs);
	*newUnit = _cyverse_logic_getNewAVUSetting(*origUnit, 'u:', *newArgs);

	_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *Attr, *Val, *origUnit);
	_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *newAttr, *newVal, *newUnit);
}

# This rule checks that AVU being added, set or removed isn't a protected one.
# Only rodsadmin users are allowed to add, remove or update protected AVUs.
cyverse_logic_acPreProcForModifyAVUMetadata(*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit) {
	if (*Opt != 'adda') {
		_cyverse_logic_ensureAVUEditable($userNameClient, $rodsZoneClient, *Attr, *Val, *Unit);
	}
}

# This rule ensures that only the non-protected AVUs are copied from one item to
# another.
cyverse_logic_acPreProcForModifyAVUMetadata(*Opt, *SrcType, *TgtType, *SrcName, *TgtName) {
	if (!canModProtectedAVU($userNameClient, $rodsZoneClient)) {
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

# This rule sends a message indicating that an AVU was modified.
cyverse_logic_acPostProcForModifyAVUMetadata(
	*Opt, *EntityType, *EntityName, *Attr, *Val, *UnitOrNew1, *New2, *New3, *New4
) {
	*newArgs = list(*UnitOrNew1, *New2, *New3, *New4);
	*uuid = '';
	_cyverse_logic_ensureUUID(*EntityType, *EntityName, $userNameClient, $rodsZoneClient, *uuid);

	# Determine the original unit and the new AVU settings.
	*origUnit = _cyverse_logic_getOrigUnit(*UnitOrNew1);
	*newAttr = _cyverse_logic_getNewAVUSetting(*Attr, 'n:', *newArgs);
	*newVal = _cyverse_logic_getNewAVUSetting(*Val, 'v:', *newArgs);
	*newUnit = _cyverse_logic_getNewAVUSetting(*origUnit, 'u:', *newArgs);

	# Send AVU modified message.
	_cyverse_logic_sendAVUMod(
		*EntityType,
		*uuid,
		*Attr,
		*Val,
		*origUnit,
		*newAttr,
		*newVal,
		*newUnit,
		$userNameClient,
		$rodsZoneClient );
}

# This rule sends one of the AVU metadata set messages, depending on which
# subcommand was used.
cyverse_logic_acPostProcForModifyAVUMetadata(*Opt, *EntityType, *EntityName, *Attr, *Val, *Unit) {
	if (*Attr != _cyverse_logic_UUID_ATTR) {
		if (_cyverse_logic_contains(*Opt, list('add', 'adda', 'rm', 'set'))) {
			*uuid = '';

			_cyverse_logic_ensureUUID(
				*EntityType, *EntityName, $userNameClient, $rodsZoneClient, *uuid );

			_cyverse_logic_sendAVUSet(
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

# This rules sends an AVU metadata copy message.
cyverse_logic_acPostProcForModifyAVUMetadata(*Opt, *SrcType, *TgtType, *SrcName, *TgtName) {
	if (cyverse_isFSType(*TgtType) && !cyverse_inStaging(/*TgtName)) {
		*tgt = _cyverse_logic_resolveMsgEntityId(
			*TgtType, *TgtName, $userNameClient, $rodsZoneClient );

		if (cyverse_isFSType(*SrcType) && !cyverse_inStaging(/*SrcName)) {
			*src = _cyverse_logic_resolveMsgEntityId(
				*SrcType, *SrcName, $userNameClient, $rodsZoneClient );

			_cyverse_logic_sendAVUCp(*SrcType, *src, *TgtType, *tgt, $userNameClient, $rodsZoneClient);
		} else {
			*uuidAttr = _cyverse_logic_UUID_ATTR;

			if (cyverse_isResc(*SrcType)) {
				foreach( *rec in
					SELECT META_RESC_ATTR_NAME, META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
					WHERE RESC_NAME == *SrcName AND META_RESC_ATTR_NAME != *uuidAttr
				) {
					_cyverse_logic_sendAVUSet(
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
					_cyverse_logic_sendAVUSet(
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
# pushes a collection.add message into the irods exchange.
cyverse_logic_acCreateCollByAdmin(*ParCollPath, *CollName) {
	*coll = str(*ParCollPath) ++ '/' ++ *CollName;
	*perm = _cyverse_logic_resolveAdmPerm(*coll);
	msiSetACL('default', 'admin:*perm', 'rodsadmin', *coll);
}

cyverse_logic_acCreateCollByAdminArchive(*ParCollPath, *CollName) {
	*path = str(*ParCollPath) ++ '/' ++ *CollName;
	*id = '';
	_cyverse_logic_ensureUUID(cyverse_COLL, *path, $userNameClient, $rodsZoneClient, *id);
	_cyverse_logic_sendCollAdd(*id, *path, $userNameClient, $rodsZoneClient);
}

# This rule makes the admin owner of any created collection. This rule is not
# applied to collections created when a TAR file is expanded. (i.e. ibun -x)
cyverse_logic_acPostProcForCollCreate {
	_cyverse_logic_setAdmPerm($collName);
}

# This rule ensures that archival collections are given a UUID and an AMQP
# message is published indicating the collection is created.
cyverse_logic_acPostProcForCollCreateArchive {
	*id = '';
	_cyverse_logic_ensureUUID(cyverse_COLL, $collName, $userNameClient, $rodsZoneClient, *id);
	_cyverse_logic_sendCollAdd(*id, $collName, $userNameClient, $rodsZoneClient);
}

# This rule pushes a collection.rm message into the irods exchange.
cyverse_logic_acDeleteCollByAdmin(*ParCollPath, *CollName) {
	*path = str(*ParCollPath) ++ '/' ++ *CollName;
	*uuid = _cyverse_logic_getCollUUID(*path);

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(cyverse_COLL, *uuid, *path, $userNameClient, $rodsZoneClient);
	}
}

cyverse_logic_acPreprocForRmColl {
	temporaryStorage.'$collName' = _cyverse_logic_getCollUUID($collName);
}

cyverse_logic_acPostProcForRmColl {
	*uuid = temporaryStorage.'$collName';

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(cyverse_COLL, *uuid, $collName, $userNameClient, $rodsZoneClient);
	}
}

# Refuse SSL connections
cyverse_logic_acPreConnect(*OUT) {
	*OUT = 'CS_NEG_REFUSE';
}

# This rule ensures that the storage resource free space is updated when a
# data object is replicated to it.
#
cyverse_logic_acPostProcForDataCopyReceived(*StoreResc) {
	msi_update_unixfilesystem_resource_free_space(*StoreResc);
}

cyverse_logic_acDataDeletePolicy {
	temporaryStorage.'$objPath' = _cyverse_logic_getDataObjUUID($objPath);
}

cyverse_logic_acPostProcForDelete {
	*uuid = temporaryStorage.'$objPath';

	if (*uuid != '') {
		_cyverse_logic_sendEntityRm(
			cyverse_DATA_OBJ, *uuid, $objPath, $userNameClient, $rodsZoneClient );
	}
}

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
cyverse_logic_acSetNumThreads {
	msiSetNumThreads('default', 'default', 'default');
}

# Whenever a large file is uploaded, recheck the free space on the storage
# resource server where the file was written.
cyverse_logic_acPostProcForParallelTransferReceived(*StoreResc) {
	msi_update_unixfilesystem_resource_free_space(*StoreResc);
}

# Set maximum number of rule engine processes
cyverse_logic_acSetReServerNumProc {
	msiSetReServerNumProc(str(cyverse_MAX_NUM_RE_PROCS));
}

# Create a user for a Data Store service
cyverse_logic_acCreateUser {
	msiCreateUser ::: msiRollback;
	msiCommit;
}


#
# DYNAMIC PEPS
#

# XXX - Because of https://github.com/irods/irods/issues/5540
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

# This rule sends a system metadata modified status message.
cyverse_logic_dataObjMetadataMod(*Username, *Zone, *DataObjPath) {
	*me = 'cyverse_logic_dataObjMetadataMod';
	*id = _cyverse_logic_getDataObjId(*DataObjPath);

	if (*id >= 0) {
		_cyverse_logic_registerAction(*id, *me);

		if (_cyverse_logic_isCurrentAction(*id, *me)) {
			*uuid = '';
			_cyverse_logic_ensureUUID(cyverse_DATA_OBJ, *DataObjPath, *Username, *Zone, *uuid);
			_cyverse_logic_sendDataObjMetadataMod(*uuid, *Username, *Zone);
			_cyverse_logic_unregisterAction(*id, *me);
		}
	}
}
