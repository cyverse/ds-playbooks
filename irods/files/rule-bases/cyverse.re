# This is a library of rules to support service specific policies.
#
# © 2023 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

@include 'cyverse-env'

_cyverse_HOME = '/' ++ cyverse_ZONE ++ '/home'

#
# These are string manipulation functions
#

# Determines whether or not the string in the first argument ends with the
# string in the second argument.
#
# Parameters:
#  Str     the string being tested
#  Suffix  the suffix being matched
#
cyverse_endsWith: string * string -> boolean
cyverse_endsWith(*Str, *Suffix) =
	if strlen(*Str) < strlen(*Suffix) then false
	else if substr(*Str, strlen(*Str) - strlen(*Suffix), strlen(*Str)) != *Suffix then false
	else true

# Determines whether or not the string in the first argument starts with the
# string in the second argument.
#
# Parameters:
#  Str     the string being tested
#  Prefix  the prefix to be matched
#
cyverse_startsWith: string * string -> boolean
cyverse_startsWith(*Str, *Prefix) =
	if strlen(*Str) < strlen(*Prefix) then false
	else if substr(*Str, 0, strlen(*Prefix)) != *Prefix then false
	else true

# Removes a prefix from a string.
#
# Parameters:
#  Orig      the string with the unwanted prefix
#  Prefixes  the set of prefixes to potentially remove
#
cyverse_rmPrefix: string * list string -> string
cyverse_rmPrefix(*Orig, *Prefixes) =
	if size(*Prefixes) == 0 then *Orig
	else
		if cyverse_startsWith(*Orig, hd(*Prefixes))
		then substr(*Orig, strlen(hd(*Prefixes)), strlen(*Orig))
		else cyverse_rmPrefix(*Orig, tl(*Prefixes))


#
# These are the constants used by iRODS to identity the type of an entity.
#

# Identifies a collection
cyverse_COLL: string
cyverse_COLL = '-C'

# Identifies a data object
cyverse_DATA_OBJ: string
cyverse_DATA_OBJ = '-d'

# Identifies a resource
cyverse_RESC: string
cyverse_RESC = '-R'

# Identifies a user
cyverse_USER: string
cyverse_USER = '-u'

# tests whether a given entity type identifier indicates a collection
#
# Parameters:
#  Type  the entity type identifier
#
# NB: Sometimes iRODS passes `-c` to indicate a collection
#
cyverse_isColl: string -> boolean
cyverse_isColl(*Type) = *Type == cyverse_COLL || *Type == '-c'

# tests whether a given entity type identifier indicates a data object
#
# Parameters:
#  Type  the entity type identifier
#
cyverse_isDataObj: string -> boolean
cyverse_isDataObj(*Type) = *Type == cyverse_DATA_OBJ

# tests whether a given entity type identifier indicates a collection or a data
# object
#
# Parameters:
#  Type  the entity type identifier
#
cyverse_isFSType: string -> boolean
cyverse_isFSType(*Type) = cyverse_isColl(*Type) || cyverse_isDataObj(*Type)

# tests whether a given entity type identifier indicates a resource
#
# Parameters:
#  Type  the entity type identifier
#
# NB: Sometimes iRODS passes `-r` to indicated a resource
#
cyverse_isResc: string -> boolean
cyverse_isResc(*Type) = *Type == cyverse_RESC || *Type == '-r'

# tests whether a given entity type identifier indicates a user
#
# Parameters:
#  Type  the entity type identifier
#
cyverse_isUser: string -> boolean
cyverse_isUser(*Type) = *Type == cyverse_USER

# Looks up the type of an entity
#
# PARAMETERS:
#  Entity  the name of a resource or user or the path of a collection or data
#          object
#
# RETURNS:
#  It returns the type or '' if the type of Entity can't be determined
#
cyverse_getEntityType: forall X in {path string}, X -> string
cyverse_getEntityType(*Entity) =
	let *entity = str(*Entity) in
	let *type = '' in
	let *ec = errormsg(msiGetObjType(*entity, *type), *err) in
	if *ec < 0 then let *_ = writeLine('serverLog', 'msiGetObjType(*entity) -> *ec: "*err"') in ''
	else
		if cyverse_isColl(*type) then cyverse_COLL
		else if cyverse_isDataObj(*type) then cyverse_DATA_OBJ
		else if cyverse_isResc(*type) then cyverse_RESC
		else if cyverse_isUser(*type) then cyverse_USER
		else *type

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
cyverse_isForSvc: forall X in {path string}, string * string * X -> boolean
cyverse_isForSvc(*SvcUser, *SvcColl, *Path) =
	let *path = str(*Path) in
	*path like regex _cyverse_HOME ++ '/[^/]+/*SvcColl($|/.*)'
	&& !(*path like _cyverse_HOME ++ '/*SvcUser/*')
	&& !(*path like _cyverse_HOME ++ '/shared/*')

# This rule gives access to a service for a collection and everything in it.
#
# PARAMETERS:
#  SvcUser   the iRODS user name used by the service
#  Perm      the permission to grant. It should be 'null', 'read', 'write', or
#            'own'.
#  CollPath  the path to the collection of begin given write access to
#
cyverse_giveAccessColl(*SvcUser, *Perm, *CollPath) {
	*path = str(*CollPath);
	writeLine('serverLog', 'permitting *SvcUser *Perm access to *path and everything in it');
	msiSetACL('recursive', *Perm, *SvcUser, *path);
}

# This rule gives access to a service for a data object.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  Perm     the permission to grant. It should be 'null', 'read', 'write', or
#           'own'.
#  ObjPath  the path to the data object of begin given write access to
#
cyverse_giveAccessDataObj(*SvcUser, *Perm, *ObjPath) {
	*path = str(*ObjPath);
	writeLine('serverLog', 'permitting *SvcUser *Perm access to *path');
	msiSetACL('default', *Perm, *SvcUser, *path);
}

# This rule ensures that a service user gets access to a presumably newly
# created collection if it were created inside a user collection managed by the
# service.
#
# PARAMETERS:
#  SvcUser   the iRODS user name used by the service
#  SvcColl   the name of the user collection managed by the service
#  Perm      the permission to grant. It should be 'null', 'read', 'write', or
#            'own'.
#  CollPath  the path to the collection of interest
#
cyverse_ensureAccessOnCreateColl(*SvcUser, *SvcColl, *Perm, *CollPath) {
	if (cyverse_isForSvc(*SvcUser, *SvcColl, *CollPath)) {
		cyverse_giveAccessColl(*SvcUser, *Perm, *CollPath);
	}
}

# This rule ensures that a service user gets access to a presumably newly
# created data object if it were created inside a user collection managed by the
# service.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  SvcColl  the name of the user collection managed by the service
#  Perm     the permission to grant. It should be 'null', 'read', 'write', or
#           'own'.
#  ObjPath  the path to the data object of interest
#
cyverse_ensureAccessOnCreateDataObj(*SvcUser, *SvcColl, *Perm, *ObjPath) {
	if (cyverse_isForSvc(*SvcUser, *SvcColl, *ObjPath)) {
		cyverse_giveAccessDataObj(*SvcUser, *Perm, *ObjPath);
	}
}

# This rule ensures that a service user gets access to a collection or data
# object if it has been moved into a user collection managed by the service.
#
# PARAMETERS:
#  SvcUser  the iRODS user name used by the service
#  SvcColl  the name of the user collection managed by the service
#  Perm     the permission to grant. It should be 'null', 'read', 'write', or
#           'own'.
#  OldPath  the original iRODS path to the entity
#  NewPath  the new iRODS path to the entity
#
cyverse_ensureAccessOnMv(*SvcUser, *SvcColl, *Perm, *OldPath, *NewPath) {
	if (
		!cyverse_isForSvc(*SvcUser, *SvcColl, *OldPath)
		&& cyverse_isForSvc(*SvcUser, *SvcColl, *NewPath)
	) {
		*type = cyverse_getEntityType(*NewPath);

		if (cyverse_isColl(*type)) {
			cyverse_giveAccessColl(*SvcUser, *Perm, *NewPath);
		} else if (cyverse_isDataObj(*type)) {
			cyverse_giveAccessDataObj(*SvcUser, *Perm, *NewPath);
		}
	}
}

# This rule sets a protected AVU on an entity as a rodsadmin user.
#
# PARAMETERS:
#  Entity  the name of a resource or user or the path of a collection or data
#          object
#  Attr    the protected attribute being set
#  Val     the value to set
#  Unit    the unit of the value
#
cyverse_setProtectedAVU(*Entity, *Attr, *Val, *Unit) {
	*cmdArg = execCmdArg('set');
	*typeArg = execCmdArg(cyverse_getEntityType(*Entity));
	*entityArg = execCmdArg(str(*Entity));
	*attrArg = execCmdArg(*Attr);
	*valArg = execCmdArg(*Val);
	*unitArg = execCmdArg(*Unit);
	*argStr = "*cmdArg *typeArg *entityArg *attrArg *valArg *unitArg";
	*status = errormsg(msiExecCmd('imeta-exec', *argStr, "null", "null", "null", *out), *msg);

	if (*status != 0) {
		msiGetStderrInExecCmdOut(*out, *err);
		writeLine('serverLog', "DS: Failed to set AVU *Attr on *Entity");
		writeLine('serverLog', "DS: *msg");
		writeLine('serverLog', "DS: *err");
	}

	*status;
}
