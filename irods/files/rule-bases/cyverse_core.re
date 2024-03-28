# This rule base contains the rules attached to Policy Execution Points for the
# core CyVerse Data Store policies. All policy logic is in this file or
# included by this file.
#
# © 2023 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

# All Data Store specific, environment independent logic goes in the file
# cyverse_logic.re. These rules will be called by the hooks implemented here.

# The shared logic usable by the Data Store and other service rules.
@include 'cyverse'

@include 'cyverse_logic'
@include 'ipc-repl'
@include 'ipc-trash'


# SERVICE SPECIFIC RULES
#
# Rule logic specific to supporting a service that uses the Data Store goes in
# its own file, and the file should be included in this section. Service
# specific rule logic should be implemented in a rule prefixed with the name of
# the rule file and suffixed with the name of the rule hook that will call the
# service's rule.

@include 'avra'
@include 'bisque'
@include 'coge'
@include 'mdrepo'
@include 'pire'


#
### STATIC PEPs ###
#

## SUPPORTING FUNCTIONS AND RULES ##

# EXCLUSIVE RULES
#
# For events occur that should belong to one and only one project,
# the following rules may be extended with on conditions.

# This rule applies the project specific collection creation policies to a newly
# created collection that wasn't created administratively.
#
cyverse_core_acPostProcForCollCreate_exclusive {
	*err = errormsg(bisque_acPostProcForCollCreate, *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(coge_acPostProcForCollCreate, *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
}


# POLICY

# This rule administratively creates a collection, e.g., creating a home
# collection when a user is created. It ensures all collection creation policies
# are applied to then newly created collection.
#
# Parameters:
#  ParColl    the absolute path to the parent of the collection being created
#  ChildColl  the name of the collection being created
#
# Error Codes:
#  -43000 (SYS_NO_RCAT_SERVER_ERR)
#  -160000 (SYS_SERVICE_ROLE_NOT_SUPPORTED)
#
acCreateCollByAdmin(*ParColl, *ChildColl) {
	msiCreateCollByAdmin(*ParColl, *ChildColl);
	cyverse_logic_acCreateCollByAdmin(*ParColl, *ChildColl);
}

# This rule handles the creation of a ds-service type user.
#
# Session Variables:
#  otherUserType
#
acCreateUser {
	on ($otherUserType == 'ds-service') {
		cyverse_logic_acCreateUser;
	}
}

# This rule applies the project specific data delete policies.
#
acDataDeletePolicy {
	bisque_acDataDeletePolicy;
	cyverse_logic_acDataDeletePolicy;
}

# This rule applies the collection delete policies for a collection being
# administratively deleted.
#
# Parameters:
#  ParColl    (string) the absolute path to the parent collection of the
#              collection being deleted
#  ChildColl  (string) the name of collection being deleted
#
acDeleteCollByAdmin(*ParColl, *ChildColl) {
	msiDeleteCollByAdmin(*ParColl, *ChildColl);
	cyverse_logic_acDeleteCollByAdmin(*ParColl, *ChildColl);
}

# This rule applies the collection delete polices for a home or trash collection
# being administratively deleted. This rule overrides the
# acDeleteCollByAdminIfPresent rule in core.re. It is called indirectly by the
# acDeleteUser PEP through the corresponding rule in core.re.
#
# Parameters:
#  ParColl    (string) the absolute path to the parent collection of the
#             collection being deleted
#  ChildColl  (string) the name of collection being deleted
#
acDeleteCollByAdminIfPresent(*ParColl, *ChildColl) {
	*status = errormsg(ipc_acDeleteCollByAdmin(*ParColl, *ChildColl), *msg);
	if(*status < 0) {
		writeLine('serverLog', *msg);
	}
	*status = errormsg(msiDeleteCollByAdmin(*ParColl, *ChildColl),*msg);
	if(*status != 0 && *status != -808000) {
		failmsg(*status, *msg);
	}
}

# This rule returns the connection encryption policy to a connection being
# established.
#
# Parameters:
#  OUT  (string) the policy name, "CS_NEG_REFUSE". "CS_NEG_REQUIRE", or
#       "CS_NEG_DONT_CARE"
#
acPreConnect(*OUT) {
	cyverse_logic_acPreConnect(*OUT);
}

# This rule sets the default values for parameters related to parallel transfer.
#
acSetNumThreads {
	cyverse_logic_acSetNumThreads;
}

# This rule sets the maximum number of deferred rule executors.
#
acSetRescSchemeForCreate {
	ipcRepl_acSetRescSchemeForCreate;
}

# This rule sets the default resource selection scheme for the replica of a
# newly created data object.
#
acSetRescSchemeForRepl {
	ipcRepl_acSetRescSchemeForRepl;
}

# This rule sets the default resource selection schema for replication of an
# existing data object.
#
acSetReServerNumProc {
	cyverse_logic_acSetReServerNumProc;
}


#
# PRE-PROC
#
# The first custom pre-proc rule that fails should cause the rest to not be
# executed. Third party pre-proc rule effects should be rolled back if a
# subsequent pre-proc rule fails. Third party pre-proc rules should be called
# before any CyVerse pre-proc rules to ensure that third party rules don't
# invalidate CyVerse rules.

# This rule sets the preprocessing policy for access control modification.
#
# Parameters:
#  RecursiveFlag  (string) indicates if the permission change applies
#                 recursively to the contents of a *Path, "1" indicates the flag
#                 is present, and "0" indicates the opposite.
#  AccessLevel    (string) the permission being granted to *UserName, if the
#                 value is "null", "read", "write", or "own", enable inheritance
#                 if the value is "inherit", or disable inheritance if the value
#                 is "noinherit"
#  UserName       (string) the account or group being given *AccessLvl, ignored
#                 if *AccessLvl is "inherit" or "noinherit"
#  Zone           (string) the zone where *UserName belongs, ignored if
#                 *AccessLvl in "inherit" or "noinherit"
#  *Path          (string) the path to the collection or data object whose ACL
#                 is being altered
#
acPreProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
	cyverse_logic_acPreProcForModifyAccessControl(
		*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path );
}

# This rule sets the preprocessing policy for manipulating AVUs other than
# copying and modification.
#
# Parameters:
#  Option    (string) the subCommand, one of 'add', 'adda', 'addw', 'rm', 'rmw'
#            'rmi', or 'set'
#  ItemType  (string) the type of entity whose AVU is being modified, '-C' for
#            collection, '-d' for data object, '-R' for resource, or '-u' for
#            user
#  ItemName  (string) the name of the entity whose AVU is being modified, this
#            is an absolute path for a collection or data object
#  AName     (string) the name of the attribute
#  AValue    (string) the value of the attribute
#  AUnit     (string) the unit of the attribute
#
acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
	cyverse_logic_acPreProcForModifyAVUMetadata(
		*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit );
}

# This rule sets the preprocessing policy for modifying AVUs.
#
# Parameters:
#  Option    (string) the value 'mod'
#  ItemType  (string) the type of entity whose AVU is being modified, '-C' for
#            collection, '-d' for data object, '-R' for resource, or '-u' for
#            user
#  ItemName  (string) the name of the entity whose AVU is being modified, this
#            is an absolute path for a collection or data object
#  AName     (string) the attribute name before modification
#  AValue    (string) the attribute value before modification
#  New1      (string) if a attribute has a unit before modification, this
#            parameter holds that unit, otherwise, it holds an update to the
#            name, value, or unit prefixed by 'n:', 'v:', or 'u:', respectively
#  New2      (string) either empty or holds an update to the name, value, or
#            unit prefixed by 'n:', 'v:', or 'u:', respectively
#  New3      (string) either empty or holds an update to the name, value, or
#            unit prefixed by 'n:', 'v:', or 'u:', respectively
#  New4      (string) either empty or holds an update to the name, value, or
#            unit prefixed by 'n:', 'v:', or 'u:', respectively
#
acPreProcForModifyAVUMetadata(
	*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName, *NAValue, *NAUnit
) {
	cyverse_logic_acPreProcForModifyAVUMetadata(
		*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName, *NAValue, *NAUnit );
}

# This rule sets the preprocessing policy for copying AVUs between entities.
#
# Parameters:
#  Option          (string) the value 'cp'
#  SourceItemType  (string) the type of entity whose AVUs are being copied, '-C'
#                   for collection, '-d' for data object, '-R' for resource, or
#                   '-u' for user
#  TargetItemType  (string) the type of entity receiving the AVUs, '-C' for
#                  collection, '-d' for data object, '-R' for resource, or '-u'
#                  for user
#  SourceItemName  (string) the name of the entity whose AVUs are being copied,
#                  for a collection or data object, this is the entity's
#                  absolute path
#  TargetItemName  (string) the name of the entity receiving the AVUs, for a
#                  collection or data object, this is the entity's absolute path
#
acPreProcForModifyAVUMetadata(
	*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName
) {
	cyverse_logic_acPreProcForModifyAVUMetadata(
		*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName );
}

# This rule sets the preprocessing policy for deleting a collection.
#
acPreprocForRmColl {
	cyverse_logic_acPreprocForRmColl;
}


# POST-PROC
#
# Post-proc rules cannot be rolled back on failure, so all custom post-proc
# rules should always be called. Third party post-proc rules should be called
# before any CyVerse post-proc rules to ensure that third party rules don't
# invalidate CyVerse rules.

# This rule sets the post-processing policy for a newly created collection.
#
acPostProcForCollCreate {
	*err = errormsg(cyverse_logic_acPostProcForCollCreate, *msg);
	if (*err < 0) { writeLine('serverLog', *msg); }

	cyverse_core_acPostProcForCollCreate_exclusive;
}

# This rule sets the post-processing policy for when a data object's replica is
# received due to a copy operation.
#
# Parameters:
#  LeafResource  (string) the name of the storage resource where the replica was
#                stored
#
acPostProcForDataCopyReceived(*LeafResource) {
	cyverse_logic_acPostProcForDataCopyReceived(*LeafResource);
}

# This rule sets the post-processing policy for deleting a data object.
#
acPostProcForDelete {
	*err = errormsg(cyverse_logic_acPostProcForDelete, *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(bisque_acPostProcForDelete, *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
}

# This rule sets the post-processing policy for an ACL change.
#
# Parameters:
#  RecursiveFlag  (string) indicates if the permission change applied
#                 recursively to the contents of a *Path, "1" indicates the flag
#                 was present, and "0" indicates the opposite.
#  AccessLevel    (string) the permission granted to *UserName, if the value was
#                 "null", "read", "write", or "own", enabled inheritance if the
#                 value was "inherit", or disabled inheritance if the value was
#                 "noinherit"
#  UserName       (string) the account or group given *AccessLevel, ignored if
#                 *AccessLevel was "inherit" or "noinherit"
#  Zone           (string) the zone where *UserName belongs, ignored if
#                 *AccessLevel in "inherit" or "noinherit"
#  Path           (string) the path to the collection or data object whose ACL
#                  was altered
#
acPostProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
	cyverse_logic_acPostProcForModifyAccessControl(
		*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path );
}

# This rule sets the post-processing policy for manipulating AVUs other than
# copying and modification.
#
# Parameters:
#  Option    (string) the subCommand, one of 'add', 'adda', 'addw', 'rm', 'rmw'
#            'rmi', or 'set'
#  ItemType  (string) the type of entity whose AVU was modified, '-C' for
#            collection, '-d' for data object, '-R' for resource, or '-u' for
#            user
#  ItemName  (string) the name of the entity whose AVU was modified, this is an
#            absolute path for a collection or data object
#  AName     (string) the name of the attribute
#  AValue    (string) the value of the attribute
#  AUnit     (string) the unit of the attribute
#
acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
	cyverse_logic_acPostProcForModifyAVUMetadata(
		*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit );
}

# This rule sets the post-processing policy for modifying AVUs.
#
# Parameters:
#  Option    (string) the value 'mod'
#  ItemType  (string) the type of entity whose AVU was modified, '-C' for
#            collection, '-d' for data object, '-R' for resource, or '-u' for
#            user
#  ItemName  (string) the name of the entity whose AVU was modified, this is an
#            absolute path for a collection or data object
#  AName     (string) the attribute name before modification
#  AValue    (string) the attribute value before modification
#  New1      (string) if a attribute has a unit before modification, this
#            parameter holds that unit, otherwise, it holds the updated name,
#            value, or unit prefixed by 'n:', 'v:', or 'u:', respectively
#  New2      (string) either empty or holds the updated name, value, or unit
#            prefixed by 'n:', 'v:', or 'u:', respectively
#  New3      (string) either empty or holds the updated name, value, or unit
#            prefixed by 'n:', 'v:', or 'u:', respectively
#  New4      (string) either empty or holds the updated name, value, or unit
#            prefixed by 'n:', 'v:', or 'u:', respectively
#
acPostProcForModifyAVUMetadata(
	*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName, *NAValue, *NAUnit
) {
	cyverse_logic_acPostProcForModifyAVUMetadata(
		*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName, *NAValue, *NAUnit );
}

# This rule sets the post-processing policy for copying AVUs between entities.
#
# Parameters:
#  Option          (string) the value 'cp'
#  SourceItemType  (string) the type of entity whose AVUs were copied, '-C' for
#                  collection, '-d' for data object, '-R' for resource, or '-u'
#                  for user
#  TargetItemType  (string) the type of entity that received the AVUs, '-C' for
#                  collection, '-d' for data object, '-R' for resource, or '-u'
#                  for user
#  SourceItemName  (string) the name of the entity whose AVUs were copied, for
#                  a collection or data object, this is the entity's absolute
#                  path
#  TargetItemName  (string) the name of the entity that received the AVUs, for a
#                  collection or data object, this is the entity's absolute path
#
acPostProcForModifyAVUMetadata(
	*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName
) {
	cyverse_logic_acPostProcForModifyAVUMetadata(
		*Option, *SourceItemType, *TargetItemType, *SourceItemName, *TargetItemName );
}

# This rule sets the post-processing policy for a moved or renamed collection or
# data object.
#
# Parameters:
#  SourceObject  (string) the path to the collection or data object prior to
#                being moved or renamed.
#  DestObject    (string) the new path
#
acPostProcForObjRename(*SourceObject, *DestObject) {
	*err = errormsg(cyverse_logic_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(bisque_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(coge_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(replEntityRename(*SourceObject, *DestObject), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
}

# This rule sets the post-processing policy for when a data object is opened.
#
# Session Variables:
#  objPath
#
acPostProcForOpen {
	*err = errormsg(cyverse_logic_acPostProcForOpen, *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
}

# This rule sets the post-processing policy for when a data object is uploaded
# via parallel transport.
#
# Parameters:
#  LeafResource  (string) the name of the storage resource where the replica was
#                stored
#
acPostProcForParallelTransferReceived(*LeafResource) {
	cyverse_logic_acPostProcForParallelTransferReceived(*LeafResource);
}

# Ths rule sets the post-processing policy for when a collection is removed.
#
acPostProcForRmColl {
	cyverse_logic_acPostProcForRmColl;
}


#
### DYNAMIC PEPS ###
#

## API ##

# COLL_CREATE

# This is the post processing logic for when a collection is created through the
# API using a COLL_CREATE request.
#
#  Instance       (string) unknown
#  Comm           (`KeyValuePair_PI`) user connection and auth information
#  CollCreateInp  (`KeyValuePair_PI`) information related to the new collection
#
pep_api_coll_create_post(*Instance, *Comm, *CollCreateInp) {
	ipcTrash_api_coll_create_post(*Instance, *Comm, *CollCreateInp);
}


# DATA_OBJ_COPY

# This is the post processing logic for when a data object is copied through the
# API using a DATA_OBJ_COPY request.
#
#  Instance        (string) unknown
#  Comm            (`KeyValuePair_PI`) user connection and auth information
#  DataObjCopyInp  (`KeyValuePair_PI`) information related to copy operation
#  TransStat       unknown
#
pep_api_data_obj_copy_post(*Instance, *Comm, *DataObjCopyInp, *TransStat) {
	ipcTrash_api_data_obj_copy_post(*Instance, *Comm, *DataObjCopyInp, *TransStat);
}


# DATA_OBJ_CREATE

# This is the post processing logic for when a data object is created through
# API using a DATA_OBJ_CREATE request.
#
#  Instance    (string) unknown
#  Comm        (`KeyValuePair_PI`) user connection and auth information
#  DataObjInp  (`KeyValuePair_PI`) information related to the created data
#              object
#
pep_api_data_obj_create_post(*Instance, *Comm, *DataObjInp) {
	ipcTrash_api_data_obj_create_post(*Instance, *Comm, *DataObjInp);
}


# DATA_OBJ_OPEN

# This is the pre processing logic for when an attempt is made to open a data
# object through the API using a DATA_OBJ_OPEN request.
#
#  Instance    (string) unknown
#  Comm        (`KeyValuePair_PI`) user connection and auth information
#  DataObjInp  (`KeyValuePair_PI`) information related to the data object
#
pep_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
	mdrepo_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp);
}


# DATA_OBJ_PUT

# This is the pre processing logic for when an attempt is made to upload a data
# object through the API using a DATA_OBJ_PUT request.
#
#  Instance        (string) unknown
#  Comm            (`KeyValuePair_PI`) user connection and auth information
#  DataObjInp      (`KeyValuePair_PI`) information related to the data object
#  DataObjInpBBuf  (unknown) may contain the contents of the file being uploaded
#  PORTAL_OPR_OUT  unknown
#
pep_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT) {
	mdrepo_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT);
}

# This is the post processing logic for when a data object is uploaded through
# the API using a DATA_OBJ_PUT request.
#
#  Instance        (string) unknown
#  Comm            (`KeyValuePair_PI`) user connection and auth information
#  DataObjInp      (`KeyValuePair_PI`) information related to the data object
#  DataObjInpBBuf  (unknown) may contain the contents of the file being uploaded
#  PORTAL_OPR_OUT  unknown
#
pep_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT) {
	ipcTrash_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT);
}


# DATA_OBJ_RENAME

# This is the pre processing logic for when an attempt is made to rename a data
# object through the API using a DATA_OBJ_RENAME request.
#
#  Instance          (string) unknown
#  Comm              (`KeyValuePair_PI`) user connection and auth information
#  DataObjRenameInp  (`KeyValuePair_PI`) information about the data object and
#                    its new path
#
pep_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp) {
	ipcTrash_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp);
}

# This is the post processing logic for when a data object is renamed through
# the API using a DATA_OBJ_RENAME request.
#
#  Instance          (string) unknown
#  Comm              (`KeyValuePair_PI`) user connection and auth information
#  DataObjRenameInp  (`KeyValuePair_PI`) information about the data object and
#                    its old path
#
pep_api_data_obj_rename_post(*Instance, *Comm, *DataObjRenameInp) {
	ipcTrash_api_data_obj_rename_post(*Instance, *Comm, *DataObjRenameInp);
}


# DATA_OBJ_UNLINK

# This is the pre processing logic for when an attempt is made to delete a data
# object through the API using a DATA_OBJ_UNLINK request.
#
#  Instance          (string) unknown
#  Comm              (`KeyValuePair_PI`) user connection and auth information
#  DataObjUnlinkInp  (`KeyValuePair_PI`) information about the data object being
#                    deleted
#
pep_api_data_obj_unlink_pre(*Instance, *Comm, *DataObjUnlinkInp) {
	ipcTrash_api_data_obj_unlink_pre(*Instance, *Comm, *DataObjUnlinkInp);
}

# This is the post processing logic for when a data object is deleted through
# the API using a DATA_OBJ_UNLINK request.
#
#  Instance          (string) unknown
#  Comm              (`KeyValuePair_PI`) user connection and auth information
#  DataObjUnlinkInp  (`KeyValuePair_PI`) information about the data object being
#                    deleted
#
pep_api_data_obj_unlink_post(*Instance, *Comm, *DataObjUnlinkInp) {
	ipcTrash_api_data_obj_unlink_post(*Instance, *Comm, *DataObjUnlinkInp);
}

# This is the exception logic for when an API DATA_OBJ_UNLINK request fails.
#
#  Instance          (string) unknown
#  Comm              (`KeyValuePair_PI`) user connection and auth information
#  DataObjUnlinkInp  (`KeyValuePair_PI`) information about the data object being
#                    deleted
#
pep_api_data_obj_unlink_except(*Instance, *Comm, *DataObjUnlinkInp) {
	ipcTrash_api_data_obj_unlink_except(*Instance, *Comm, *DataObjUnlinkInp);
}


# RM_COLL

# This is the pre processing logic for when an attempt is made to delete a
# collection through the API using a RM_COLL request.
#
#  Instance     (string) unknown
#  Comm         (`KeyValuePair_PI`) user connection and auth information
#  RmCollInp    (`KeyValuePair_PI`) information about the collection being
#               deleted
#  CollOprStat  unknown
#
pep_api_rm_coll_pre(*Instance, *Comm, *RmCollInp, *CollOprStat) {
	ipcTrash_api_rm_coll_pre(*Instance, *Comm, *RmCollInp, *CollOprStat);
}

# This is the exception logic for when an API RM_COLL request fails.
#
#  Instance     (string) unknown
#  Comm         (`KeyValuePair_PI`) user connection and auth information
#  RmCollInp    (`KeyValuePair_PI`) information about the collection being
#               deleted
#  CollOprStat  unknown
#
pep_api_rm_coll_except(*Instance, *Comm, *RmCollInp, *CollOprStat) {
	ipcTrash_api_rm_coll_except(*Instance, *Comm, *RmCollInp, *CollOprStat);
}


## DATABASE ##

## SUPPORTING FUNCTIONS AND RULES ##

_cyverse_core_getObjPath(*DataObjInfo) =
	let *path = *DataObjInfo.logical_path in
	let *_ = if (*path == '') {
		*id = *DataObjInfo.data_id;
		foreach (*rec in SELECT COLL_NAME, DATA_NAME WHERE DATA_ID = *id) {
			*path = *rec.COLL_NAME ++ '/' ++ *rec.DATA_NAME;
		} } in
	/*path

# generates a unique session variable name for a data object
#
# Parameters:
#  Path  the absolute path to the data object
#
# Return:
#  the session variable name
#
#_cyverse_core_mkDataObjSessVar: path -> string
_cyverse_core_mkDataObjSessVar(*Path) = 'ipc-data-obj-' ++ str(*Path)

# XXX - Because of https://github.com/irods/irods/issues/5540
# _cyverse_core_dataObjCreated(*User, *Zone, *DataObjInfo) {
# 	*path = *DataObjInfo.logical_path;
# 	*err = errormsg(cyverse_logic_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
# 	if (*err < 0) {
# 		writeLine('serverLog', *msg);
# 	}
# 	*err = errormsg(bisque_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
# 	if (*err < 0) {
# 		writeLine('serverLog', *msg);
# 	}
# 	*err = errormsg(coge_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
# 	if (*err < 0) {
# 		writeLine('serverLog', *msg);
# 	}
# 	*err = errormsg(ipcRepl_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
# 	if (*err < 0) {
# 		writeLine('serverLog', *msg);
#	}
# }
_cyverse_core_dataObjCreated(*User, *Zone, *DataObjInfo, *Step) {
	*path = *DataObjInfo.logical_path;
	*err = errormsg(cyverse_logic_dataObjCreated(*User, *Zone, *DataObjInfo, *Step), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	if (*Step != 'FINISH') {
		*err = errormsg(bisque_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
		if (*err < 0) {
			writeLine('serverLog', *msg);
		}
		*err = errormsg(coge_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
		if (*err < 0) {
			writeLine('serverLog', *msg);
		}
	}
	if (*Step != 'START') {
		*err = errormsg(ipcRepl_dataObjCreated(*User, *Zone, *DataObjInfo), *msg);
		if (*err < 0) {
			writeLine('serverLog', *msg);
		}
	}
}
# XXX - ^^^

_cyverse_core_dataObjModified(*User, *Zone, *DataObjInfo) {
	*path = *DataObjInfo.logical_path;
	*err = errormsg(cyverse_logic_dataObjMod(*User, *Zone, *DataObjInfo), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
	*err = errormsg(ipcRepl_dataObjModified(*User, *Zone, *DataObjInfo), *msg);
	if (*err < 0) {
		writeLine('serverLog', *msg);
	}
}

_cyverse_core_dataObjMetadataModified(*User, *Zone, *Object) {
	cyverse_logic_dataObjMetaMod(*User, *Zone, *Object);
}

# CLOSE

# The rule handles the post-processing logic for when a database connection is
# closed.
#
# Parameters:
#  Instance  (string) the type of DBMS being used
#  Context   (`KeyValuePair_PI`) the database plugin context
#  OUT       (`KeyValuePair_PI`) unused
#
pep_database_close_post(*Instance, *Context, *OUT) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# nothing can be done here
# 	foreach (*key in temporaryStorage) {
# 		*values = split(temporaryStorage.'*key', ' ');
# # XXX - Because of https://github.com/irods/irods/issues/5538, the Context
# # variables need to passed through temporaryStorage
# # 		*user = *Context.user_user_name
# # 		*zone = *Context.user_rods_zone
# # 		*doiMspStr = triml(temporaryStorage.'*key', ' ');
# 		*user = elem(*values, 1);
# 		*zone = elem(*values, 2);
# 		*doiMspStr = triml(triml(triml(temporaryStorage.'*key', ' '), ' '), ' ');
# # XXX - ^^^
# 		*doiKvs = split(*doiMspStr, '++++');
# 		*op = elem(*values, 0);
# 		*doiStr = '';
# 		foreach (*kv in *doiKvs) {
# 			*doiStr = if *doiStr == '' then *kv else *doiStr ++ '%' ++ *kv;
# 		}
# 		msiString2KeyValPair(*doiStr, *doi);
# 		if (*op == 'CREATE') {
# 			_cyverse_core_dataObjCreated(*user, *zone, *doi);
# 		} else if (*op == 'MODIFY') {
# 			_cyverse_core_dataObjModified(*user, *zone, *doi);
# 		}
# 	}
}

# The rule handles the final logic for when a database connection is closed.
#
# Parameters:
#  Instance  (string) the type of DBMS being used
#  Context   (`KeyValuePair_PI`) the database plugin context
#  OUT       (`KeyValuePair_PI`) unused
#
pep_database_close_finally(*Instance, *Context, *OUT) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# cleanup can't happen
# 	foreach (*key in temporaryStorage) {
# 		temporaryStorage.'*key' = '';
# 	}
}


# MOD DATA OBJ META

# The rule handles the post-processing logic for when data object system
# metadata are modified in the catalog.
#
# Parameters:
#  Instance     (string) the type of DBMS being used
#  Context      (`KeyValuePair_PI`) the database plugin context
#  OUT          (`KeyValuePair_PI`) unused
#  DataObjInfo  (`KeyValuePair_PI`) the DATA_OBJ_INFO map for the event
#  RegParam     (`KeyValuePair_PI`) the metadata updates
#
# temporaryStorage:
#  ipc-data-obj-*DataObjInfo.logical_path          this value is read to
#                                                  determine whether a system
#                                                  metadata modification or a
#                                                  data object was created
#  ipc-data-obj-XXX5540:*DataOBjInfo.logical_path  this value is read to
#                                                  determine what the transfer
#                                                  stage is for a newly created
#                                                  data object
#
pep_database_mod_data_obj_meta_post(*Instance, *Context, *OUT, *DataObjInfo, *RegParam) {
	*handled = false;
	*logicalPath = _cyverse_core_getObjPath(*DataObjInfo);
# XXX - Because of https://github.com/irods/irods/issues/5540,
# _cyverse_core_dataObjCreated needs to be called here when not created through file
# registration
	if (! *handled && errorcode(*RegParam.dataSize) == 0) {
		*pathVar = _cyverse_core_mkDataObjSessVar(*logicalPath);
		if (
			(
				if errorcode(temporaryStorage.'*pathVar') == 0
				then temporaryStorage.'*pathVar' like 'CREATE *'
				else false
			) && (
				if errorcode(temporaryStorage.'XXX5540:*pathVar') == 0
				then temporaryStorage.'XXX5540:*pathVar' like 'START *'
				else false
			)
		) {
			*parts = split(temporaryStorage.'XXX5540:*pathVar', ' ');
			*DataObjInfo.data_owner_name = elem(*parts, 1);
			*DataObjInfo.data_owner_zone = elem(*parts, 2);
			_cyverse_core_dataObjCreated(
				*Context.user_user_name, *Context.user_rods_zone, *DataObjInfo, 'FINISH' );
			*handled = true;
		}
	}
# XXX - ^^^
	# If modification timestamp is being modified, the data object has been
	# modified, so publish a data-object.mod message.
	if (! *handled && errorcode(*RegParam.dataModify) == 0) {
		*pathVar = _cyverse_core_mkDataObjSessVar(*logicalPath);
		if (
			if errorcode(temporaryStorage.'*pathVar') != 0 then true
			else ! (temporaryStorage.'*pathVar' like 'CREATE *')
		) {
			# If RegParam.allReplStatus is TRUE, then this is a modification and not
			# a replica update.
			if (
				if errorcode(*RegParam.allReplStatus) != 0 then false
				else *RegParam.allReplStatus == 'TRUE'
			) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# _cyverse_core_dataObjModified needs to be called here
# # XXX - Because of https://github.com/irods/irods/issues/5538, the Context
# # variables need to passed through temporaryStorage
# # 				temporaryStorage.'*pathVar' = 'MODIFY *DataObjInfo';
# 				temporaryStorage.'*pathVar'
# 					= 'MODIFY '
# 					++ *Context.user_user_name
# 					++ ' '
# 					++ *Context.user_rods_zone
# 					++ ' *DataObjInfo';
# # XXX - ^^^
				_cyverse_core_dataObjModified(
					*Context.user_user_name, *Context.user_rods_zone, *DataObjInfo );
# XXX - ^^^
			}
			*handled = true;
		}
	}
# XXX - Because of https://github.com/irods/irods/issues/5584, when an expiry
# time, data type, or comment is set on a data object, sometimes *RegParam is a
# string, and we can't tell which field was set. Due to
# https://github.com/irods/irods/issues/5583, we have can't risk calling
# msiExecCmd when a data object is being overwritten using parallel transfer.
# Through experimentation, *RegParam serializes to '0', for the calls to this
# PEP that happen during a data object modification that don't set dataSize or
# dataModify.
	if (! *handled && '*RegParam' != '0') {
		_cyverse_core_dataObjMetadataModified(
			*Context.user_user_name, *Context.user_rods_zone, *logicalPath );
	}
# XXX - ^^^
}


# MOD TICKET

# This is this post processing logic for when a ticket is added to the catalog
# or modified with in the catalog.
#
#  Instance      (string) the type of DBMS being used
#  Context       (`KeyValuePair_PI`) the database plugin context
#  OUT           (`KeyValuePair_PI`) unknown
#  OpName        unknown
#  TicketString  (string) the ticket label
#  Arg3          unknown
#  Arg4          unknown
#  Arg5          unknown
#
pep_database_mod_ticket_post(
	*Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5
) {
	mdrepo_database_mod_ticket_post(
		*Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5 );
}


# REG DATA OBJ

# This is the post-processing logic for when a data object is added to the
# catalog.
#
#  Instance     (string) the type of DBMS being used
#  Context      (`KeyValuePair_PI`) the database plugin context
#  OUT          (`KeyValuePair_PI`) unused
#  DataObjInfo  (`KeyValuePair_PI`) the DATA_OBJ_INFO map for the event
#
# temporaryStorage:
#  ipc-data-obj-*DataObjInfo.logical_path          the value
#                                                  "CREATE *user *zone *DataObjInfo"
#                                                  is written here where *user
#                                                  is the user who created the
#                                                  data object and *zone is the
#                                                  user's authentication zone
#  ipc-data-obj-XXX5540:*DataOBjInfo.logical_path  the value
#                                                  "*step *owner *zone" is
#                                                  written here where *owner is
#                                                  the user who owns the data
#                                                  object, *zone is the owner's
#                                                  authentication zone, and
#                                                  *step is either "START" to
#                                                  indicate the upload is in
#                                                  progress or "FULL" to
#                                                  indicate the upload completed
#
pep_database_reg_data_obj_post(*Instance, *Context, *OUT, *DataObjInfo) {
# XXX - These fields are empty. See https://github.com/irods/irods/issues/5554
	*DataObjInfo.data_owner_name = *Context.user_user_name;
	*DataObjInfo.data_owner_zone = *Context.user_rods_zone;
# XXX - ^^^
# XXX - Because of https://github.com/irods/irods/issues/5870,
# *DataObjInfo.logical_path cannot directly be converted to a path.
# 	*pathVar = _cyverse_core_mkDataObjSessVar(/*DataObjInfo.logical_path);
	*logicalPath = *DataObjInfo.logical_path;
	*pathVar = _cyverse_core_mkDataObjSessVar(/*logicalPath);
# XXX - ^^^
# XXX - Because of https://github.com/irods/irods/issues/5538, the Context
# variables need to passed through temporaryStorage
# 	temporaryStorage.'*pathVar' = 'CREATE *DataObjInfo';
	temporaryStorage.'*pathVar'
		= 'CREATE ' ++ *Context.user_user_name ++ ' ' ++ *Context.user_rods_zone ++ ' *DataObjInfo';
# XXX - ^^^
# XXX - Because of https://github.com/irods/irods/issues/5540,
# _cyverse_core_dataObjCreated needs to be called here for data objects created when
# registering a file already on a resource server.
	# NB: When a data object is created due to file registration, the size of the
	# file is known. Almost always the size will be greater than 0. This isn't
	# good enough. We get lots of zero byte files.
	*step = if *DataObjInfo.data_size == '0' then 'START' else 'FULL';
	_cyverse_core_dataObjCreated(
		*Context.user_user_name, *Context.user_rods_zone, *DataObjInfo, *step );
	temporaryStorage.'XXX5540:*pathVar'
		= *step
		++ ' '
		++ *DataObjInfo.data_owner_name
		++ ' '
		++ *DataObjInfo.data_owner_zone;
# XXX - ^^^
}


## RESOURCE ##

# RESOLVE HIERARCHY

# This rule is provides the preprocessing logic for determine which  storage
# resource to choose for a replica. It is meant for project specific
# implementations where a project implementation is within an `on` block that
# restricts the resource resolution to entities relevant to the project.post
#
# Parameters:
#  Instance  (string) the resource being considered
#  Context   (`KeyValuePair_PI`) the resource plugin context
#  OUT       (`KeyValuePair_PI`) unused
#  Op        (string) the operation that will be performed on the replica,
#            "CREATE" for creating the replica, "OPEN" for reading the replica,
#            and "WRITE" for overwriting an existing replica.
#  Host      (string) the host executing this policy
#  PARSER    (`KeyValuePair_PI`) unused
#  VOTE      (float) unused
#
# temporaryStorage:
#  resource_resolve_hierarchy_err  this value is read to see if an error
#                                  occurred on a previously tried on branch of
#                                  this rule
#
# Error Codes:
#  -32000 (SYS_INVALID_RESC_INPUT)  this is returned when an error occurred in
#                                   one of the on branches of this rule
#
pep_resource_resolve_hierarchy_pre(*Instance, *Context, *OUT, *Operation, *Host, *PARSER, *VOTE) {
# XXX - Because of https://github.com/irods/irods/issues/6463, an error
# happening in an `ON` condition needs to be captured and sent in the catch-all.
	if (errorcode(temporaryStorage.resource_resolve_hierarchy_err) == 0) {
		failmsg(-32000, temporaryStorage.resource_resolve_hierarchy_err);
	}
# XXX - ^^^
}
