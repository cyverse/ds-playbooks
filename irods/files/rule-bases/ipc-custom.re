# This rulebase contains the rules attached to Policy Execution Points for the
# core CyVerse Data Store policies. All policy logic is in this file or
# included by this file.
#
# © 2021 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

# The environment-specific configuration constants belong in the file
# cyverse-env.re.

@include 'cyverse-env'

# All CyVerse specific, environment independent logic goes in the file
# ipc-logic.re. These rules will be called by the hooks implemented in
# ipc-custom.re. The rule names should be prefixed with 'ipc' and suffixed with
# the name of the rule hook that will call the custom rule.

# The shared logic usable by all CyVerse and third parties
@include 'ipc-services'

@include 'ipc-logic'
@include 'ipc-repl'
@include 'ipc-trash'


# THIRD PARTY RULES
#
# Third party rule logic goes in its own file, and the file should be included
# in this section. Third party rule logic should be implemented in a rule
# prefixed with the name of the rule file and suffixed with the name of the rule
# hook that will call the custome rule.

@include 'avra'
@include 'bisque'
@include 'calliope'
@include 'captcn'
@include 'coge'
@include 'de'
@include 'mdrepo'
@include 'pire'
@include 'sciapps'
@include 'sernec'
@include 'sparcd'
@include 'terraref'


# EXCLUSIVE RULES
#
# For events occur that should belong to one and only one project,
# the following rules may be extended with ON conditions.


# This rule applies the project specific collection creation policies to an
# administratively created collection.
#
# Parameters:
#  *ParColl    the absolute path to the parent of the collection being created
#  *ChildColl  the name of the collection being created
#
exclusive_acCreateCollByAdmin(*ParColl, *ChildColl) {
  ipc_archive_acCreateCollByAdmin(*ParColl, *ChildColl);
}


# This rule applies the project specific collection creation policies to a newly
# created collection that wasn't created administratively.
#
exclusive_acPostProcForCollCreate {
  *err = errormsg(ipc_archive_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(bisque_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(captcn_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(coge_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sciapps_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sernec_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sparcd_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }
}


# This rule applies the project specific policies to a data object created
# through copying another data object.
#
exclusive_acPostProcForCopy {
  *err = errormsg(captcn_acPostProcForCopy, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sernec_acPostProcForCopy, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }
}


# POLICIES


# This rule administratively creates a collection, e.g., creating a home
# collection when a user is created. It ensures all collection creation policies
# are applied to then newly created collection.
#
# Parameters:
#  *ParColl    the absolute path to the parent of the collection being created
#  *ChildColl  the name of the collection being created
#
# Error Codes:
#  -43000 (SYS_NO_RCAT_SERVER_ERR)
#  -160000 (SYS_SERVICE_ROLE_NOT_SUPPORTED)
#
acCreateCollByAdmin(*ParColl, *ChildColl) {
  msiCreateCollByAdmin(*ParColl, *ChildColl);

  *err = errormsg(ipc_acCreateCollByAdmin(*ParColl, *ChildColl), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  exclusive_acCreateCollByAdmin(*ParColl, *ChildColl);
}


acCreateUser {
  ON ($otherUserType == 'ds-service') {
    ipc_acCreateUser;
  }
}


acDataDeletePolicy {
  bisque_acDataDeletePolicy;
  ipc_acDataDeletePolicy;
}


acDeleteCollByAdmin(*ParColl, *ChildColl) {
  msiDeleteCollByAdmin(*ParColl, *ChildColl);
  ipc_acDeleteCollByAdmin(*ParColl, *ChildColl);
}


acDeleteCollByAdminIfPresent(*ParColl, *ChildColl) {
  *status = errormsg(ipc_acDeleteCollByAdmin(*ParColl, *ChildColl), *msg);
  if(*status < 0) { writeLine('serverLog', *msg); }

  *status = errormsg(msiDeleteCollByAdmin(*ParColl, *ChildColl),*msg);
  if(*status != 0 && *status != -808000) {
    failmsg(*status, *msg);
  }
}


acPreConnect(*OUT) { ipc_acPreConnect(*OUT); }


acSetNumThreads { ipc_acSetNumThreads; }


acSetRescSchemeForCreate {
  ipcRepl_acSetRescSchemeForCreate;
}


acSetRescSchemeForRepl {
  ipcRepl_acSetRescSchemeForRepl;
}


acSetReServerNumProc { ipc_acSetReServerNumProc; }


#
# PRE-PROC RULE HOOKS
#
# The first custom pre-proc rule that fails should cause the rest to not be
# executed. Third party pre-proc rule effects should be rolled back if a
# subsequent pre-proc rule fails. Third party pre-proc rules should be called
# before any IPC pre-proc rules to ensure that third party rules don't
# invalidate IPC rules.


acPreProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
  ipc_acPreProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path);
}


acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName,
                              *NAValue, *NAUnit) {
  ipc_acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit,
                                    *NAName, *NAValue, *NAUnit);
}


acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
  ipc_acPreProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit);
}


acPreProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                              *TargetItemName) {
  ipc_acPreProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                                    *TargetItemName);
}


acPreProcForObjRename(*SourceObject, *DestObject) {
  de_acPreProcForObjRename(*SourceObject, *DestObject);
}


# NOTE: The camelcasing is inconsistent here
acPreprocForRmColl { ipc_acPreprocForRmColl; }


# POST-PROC RULE HOOKS
#
# Post-proc rules cannot be rolled back on failure, so all custom post-proc
# rules should always be called. Third party post-proc rules should be called
# before any IPC post-proc rules to ensure that third party rules don't
# invalidate IPC rules.


acPostProcForCopy {
  exclusive_acPostProcForCopy;
}


acPostProcForCollCreate {
  *err = errormsg(ipc_acPostProcForCollCreate, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  exclusive_acPostProcForCollCreate;
}


acPostProcForDataCopyReceived(*leaf_resource) { ipc_acPostProcForDataCopyReceived(*leaf_resource); }


acPostProcForOpen {
  if (!ipc_inStaging(/$objPath)) {
    *err = errormsg(ipc_acPostProcForOpen, *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }
  }
}


acPostProcForRmColl { ipc_acPostProcForRmColl; }


acPostProcForDelete {
  *err = errormsg(ipc_acPostProcForDelete, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(bisque_acPostProcForDelete, *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }
}


acPostProcForObjRename(*SourceObject, *DestObject) {
  *err = errormsg(ipc_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(bisque_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(captcn_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(coge_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sciapps_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(sernec_acPostProcForObjRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }

  *err = errormsg(replEntityRename(*SourceObject, *DestObject), *msg);
  if (*err < 0) { writeLine('serverLog', *msg); }
}


acPostProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path) {
  if (!ipc_inStaging(/*Path)) {
    ipc_acPostProcForModifyAccessControl(*RecursiveFlag, *AccessLevel, *UserName, *Zone, *Path);
  }
}


acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit, *NAName,
                               *NAValue, *NAUnit) {
  ipc_acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit,
                                     *NAName, *NAValue, *NAUnit);
}


acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit) {
  ipc_acPostProcForModifyAVUMetadata(*Option, *ItemType, *ItemName, *AName, *AValue, *AUnit);
}


acPostProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                               *TargetItemName) {
  ipc_acPostProcForModifyAVUMetadata(*Option, *SourceItemType, *TargetItemType, *SourceItemName,
                                     *TargetItemName);
}


acPostProcForParallelTransferReceived(*LeafResource) {
  ipc_acPostProcForParallelTransferReceived(*LeafResource);
}


#
### DYNAMIC PEPS ###
#

## SUPPORTING FUNCTIONS AND RULES ##

_ipc_getObjPath(*DATA_OBJ_INFO) =
  let *path = *DATA_OBJ_INFO.logical_path in
  let *_ = if (*path == '') {
     *id = *DATA_OBJ_INFO.data_id;
     foreach (*rec in SELECT COLL_NAME, DATA_NAME WHERE DATA_ID = *id) {
       *path = *rec.COLL_NAME ++ '/' ++ *rec.DATA_NAME;
      } } in
  /*path

# generates a unique session variable name for a data object
#
# Parameters:
#  *Path  the absolute path to the data object
#
# Return:
#  the session variable name
#
_ipc_mkDataObjSessVar: path -> string
_ipc_mkDataObjSessVar(*Path) = 'ipc-data-obj-' ++ str(*Path)


# XXX - Because of https://github.com/irods/irods/issues/5540
# _ipc_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO) {
#   *path = *DATA_OBJ_INFO.logical_path;
#
#   if (ipc_inStaging(/*path)) {
#     *err = errormsg(ipc_dataObjCreated_staging(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(de_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#   } else {
#     *err = errormsg(ipc_dataObjCreated_default(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(bisque_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(calliope_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(coge_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(sciapps_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(sparcd_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#
#     *err = errormsg(ipcRepl_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
#     if (*err < 0) { writeLine('serverLog', *msg); }
#   }
# }
_ipc_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO, *Step) {
  *path = *DATA_OBJ_INFO.logical_path;

  if (ipc_inStaging(/*path)) {
    *err = errormsg(ipc_dataObjCreated_staging(*User, *Zone, *DATA_OBJ_INFO, *Step), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }

    if (*Step != 'START') {
      *err = errormsg(de_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }
    }
  } else {
    *err = errormsg(ipc_dataObjCreated_default(*User, *Zone, *DATA_OBJ_INFO, *Step), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }

    if (*Step != 'FINISH') {
      *err = errormsg(bisque_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }

      *err = errormsg(coge_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }

      *err = errormsg(sciapps_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }
    }

    if (*Step != 'START') {
      *err = errormsg(calliope_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }

      *err = errormsg(sparcd_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }

      *err = errormsg(ipcRepl_dataObjCreated(*User, *Zone, *DATA_OBJ_INFO), *msg);
      if (*err < 0) { writeLine('serverLog', *msg); }
    }
  }
}
# XXX - ^^^


_ipc_dataObjModified(*User, *Zone, *DATA_OBJ_INFO) {
  *path = *DATA_OBJ_INFO.logical_path;

  if (! ipc_inStaging(/*path)) {
    *err = errormsg(ipc_dataObjModified_default(*User, *Zone, *DATA_OBJ_INFO), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }

    *err = errormsg(ipcRepl_dataObjModified(*User, *Zone, *DATA_OBJ_INFO), *msg);
    if (*err < 0) { writeLine('serverLog', *msg); }
  }
}


_ipc_dataObjMetadataModified(*User, *Zone, *Object) {
  ipc_dataObjMetadataModified(*User, *Zone, *Object);
}


## API ##

# COLL_CREATE

pep_api_coll_create_post(*Instance, *Comm, *CollCreateInp) {
  ipcTrash_api_coll_create_post(*Instance, *Comm, *CollCreateInp);
}


# DATA_OBJ_COPY

pep_api_data_obj_copy_post(*Instance, *Comm, *DataObjCopyInp, *TransStat) {
  ipcTrash_api_data_obj_copy_post(*Instance, *Comm, *DataObjCopyInp, *TransStat);
}


# DATA_OBJ_CREATE

pep_api_data_obj_create_post(*Instance, *Comm, *DataObjInp) {
  ipcTrash_api_data_obj_create_post(*Instance, *Comm, *DataObjInp);
}


# DATA_OBJ_OPEN

pep_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp) {
  mdrepo_api_data_obj_open_pre(*Instance, *Comm, *DataObjInp);
}


# DATA_OBJ_PUT

pep_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PortalOprOut) {
  mdrepo_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PortalOprOut);
}

pep_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTALOPROUT) {
  ipcTrash_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTALOPROUT);
}


# DATA_OBJ_RENAME

pep_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp) {
  ipcTrash_api_data_obj_rename_pre(*Instance, *Comm, *DataObjRenameInp);
}

pep_api_data_obj_rename_post(*Instance, *Comm, *DataObjRenameInp) {
  ipcTrash_api_data_obj_rename_post(*Instance, *Comm, *DataObjRenameInp);
}


# DATA_OBJ_UNLINK

pep_api_data_obj_unlink_pre(*Instance, *Comm, *DataObjUnlinkInp) {
  ipcTrash_api_data_obj_unlink_pre(*Instance, *Comm, *DataObjUnlinkInp);
}

pep_api_data_obj_unlink_post(*Instance, *Comm, *DataObjUnlinkInp) {
  ipcTrash_api_data_obj_unlink_post(*Instance, *Comm, *DataObjUnlinkInp);
}

pep_api_data_obj_unlink_except(*Instance, *Comm, *DataObjUnlinkInp) {
  ipcTrash_api_data_obj_unlink_except(*Instance, *Comm, *DataObjUnlinkInp);
}


# RM_COLL

pep_api_rm_coll_pre(*Instance, *Comm, *RmCollInp, *CollOprStat) {
  ipcTrash_api_rm_coll_pre(*Instance, *Comm, *RmCollInp, *CollOprStat);
}

pep_api_rm_coll_except(*Instance, *Comm, *RmCollInp, *CollOprStat) {
  ipcTrash_api_rm_coll_except(*Instance, *Comm, *RmCollInp, *CollOprStat);
}


## DATABASE ##

# CLOSE

pep_database_close_post(*INSTANCE, *CONTEXT, *OUT) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# nothing can be done here
#   foreach (*key in temporaryStorage) {
#     *vals = split(temporaryStorage.'*key', ' ');
# # XXX - Because of https://github.com/irods/irods/issues/5538, the CONTEXT
# # variables need to passed through temporaryStorage
# #     *user = *CONTEXT.user_user_name
# #     *zone = *CONTEXT.user_rods_zone
# #     *doiMspStr = triml(temporaryStorage.'*key', ' ');
#     *user = elem(*vals, 1);
#     *zone = elem(*vals, 2);
#     *doiMspStr = triml(triml(triml(temporaryStorage.'*key', ' '), ' '), ' ');
# # XXX - ^^^
#     *doiKvs = split(*doiMspStr, '++++');
#     *op = elem(*vals, 0);
#
#     *doiStr = '';
#     foreach (*kv in *doiKvs) {
#       *doiStr = if *doiStr == '' then *kv else *doiStr ++ '%' ++ *kv;
#     }
#
#     msiString2KeyValPair(*doiStr, *doi);
#
#     if (*op == 'CREATE') {
#       _ipc_dataObjCreated(*user, *zone, *doi);
#     } else if (*op == 'MODIFY') {
#       _ipc_dataObjModified(*user, *zone, *doi);
#     }
#   }
}

pep_database_close_finally(*INSTANCE, *CONTEXT, *OUT) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# cleanup can't happen
#   foreach (*key in temporaryStorage) {
#     temporaryStorage.'*key' = '';
#   }
}


# MOD DATA OBJ META

pep_database_mod_data_obj_meta_post(*INSTANCE, *CONTEXT, *OUT, *DATA_OBJ_INFO, *REG_PARAM) {
  *handled = false;
  *logicalPath = _ipc_getObjPath(*DATA_OBJ_INFO);

# XXX - Because of https://github.com/irods/irods/issues/5540,
# _ipc_dataObjCreated needs to be called here when not created through file
# registration
  if (! *handled && errorcode(*REG_PARAM.dataSize) == 0) {
    *pathVar = _ipc_mkDataObjSessVar(*logicalPath);

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
      *DATA_OBJ_INFO.data_owner_name = elem(*parts, 1);
      *DATA_OBJ_INFO.data_owner_zone = elem(*parts, 2);

      _ipc_dataObjCreated(
        *CONTEXT.user_user_name, *CONTEXT.user_rods_zone, *DATA_OBJ_INFO, 'FINISH');

      *handled = true;
    }
  }
# XXX - ^^^

  # If modification timestamp is being modified, the data object has been
  # modified, so publish a data-object.mod message.
  if (! *handled && errorcode(*REG_PARAM.dataModify) == 0) {
    *pathVar = _ipc_mkDataObjSessVar(*logicalPath);

    if (
      if errorcode(temporaryStorage.'*pathVar') != 0 then true
      else ! (temporaryStorage.'*pathVar' like 'CREATE *')
    ) {
      # If REG_PARAM.allReplStatus is TRUE, then this is a modification and not
      # a replica update.
      if (
        if errorcode(*REG_PARAM.allReplStatus) != 0 then false
        else *REG_PARAM.allReplStatus == 'TRUE'
      ) {
# XXX - Because of https://github.com/irods/irods/issues/5540,
# _ipc_dataObjModified needs to be called here
# # XXX - Because of https://github.com/irods/irods/issues/5538, the CONTEXT
# # variables need to passed through temporaryStorage
# #         temporaryStorage.'*pathVar' = 'MODIFY *DATA_OBJ_INFO';
#         temporaryStorage.'*pathVar'
#           = 'MODIFY '
#           ++ *CONTEXT.user_user_name
#           ++ ' '
#           ++ *CONTEXT.user_rods_zone
#           ++ ' *DATA_OBJ_INFO';
# # XXX - ^^^
        _ipc_dataObjModified(*CONTEXT.user_user_name, *CONTEXT.user_rods_zone, *DATA_OBJ_INFO);
# XXX - ^^^
      }

      *handled = true;
    }
  }

# XXX - Because of https://github.com/irods/irods/issues/5584, when an expiry
# time, data type, or comment is set on a data object, sometimes *REG_PARAM is a
# string, and we can't tell which field was set. Due to
# https://github.com/irods/irods/issues/5583, we have can't risk calling
# msiExecCmd when a data object is being overwritten using parallel transfer.
# Through experimentation, *REG_PARAM serializes to '0', for the calls to this
# PEP that happen during a data object modification that don't set dataSize or
# dataModify.
  if (! *handled && '*REG_PARAM' != '0') {
    _ipc_dataObjMetadataModified(*CONTEXT.user_user_name, *CONTEXT.user_rods_zone, *logicalPath);
  }
# XXX - ^^^
}


# MOD TICKET

pep_database_mod_ticket_post(*Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5)
{
  mdrepo_database_mod_ticket_post(
    *Instance, *Context, *OUT, *OpName, *TicketString, *Arg3, *Arg4, *Arg5);
}


# REG DATA OBJ

pep_database_reg_data_obj_post(*INSTANCE, *CONTEXT, *OUT, *DATA_OBJ_INFO) {
# XXX - These fields are empty. See https://github.com/irods/irods/issues/5554
  *DATA_OBJ_INFO.data_owner_name = *CONTEXT.user_user_name;
  *DATA_OBJ_INFO.data_owner_zone = *CONTEXT.user_rods_zone;
# XXX - ^^^
# XXX - Because of https://github.com/irods/irods/issues/5870,
# *DATA_OBJ_INFO.logical_path cannot directly be converted to a path.
#   *pathVar = _ipc_mkDataObjSessVar(/*DATA_OBJ_INFO.logical_path);
  *logicalPath = *DATA_OBJ_INFO.logical_path;
  *pathVar = _ipc_mkDataObjSessVar(/*logicalPath);
# XXX - ^^^
# XXX - Because of https://github.com/irods/irods/issues/5538, the CONTEXT
# variables need to passed through temporaryStorage
#   temporaryStorage.'*pathVar' = 'CREATE *DATA_OBJ_INFO';
  temporaryStorage.'*pathVar'
    = 'CREATE ' ++ *CONTEXT.user_user_name ++ ' ' ++ *CONTEXT.user_rods_zone ++ ' *DATA_OBJ_INFO';
# XXX - Because of https://github.com/irods/irods/issues/5540,
# _ipc_dataObjCreated needs to be called here for data objects created when
# registering a file already on a resource server.
  # NB: When a data object is created due to file registration, the size of the
  # file is known. Almost always the size will be greater than 0. This isn't
  # good enough. We get lots of zero byte files.
  *step = if *DATA_OBJ_INFO.data_size == '0' then 'START' else 'FULL';
  _ipc_dataObjCreated(*CONTEXT.user_user_name, *CONTEXT.user_rods_zone, *DATA_OBJ_INFO, *step);

  temporaryStorage.'XXX5540:*pathVar'
    = *step
    ++ ' '
    ++ *DATA_OBJ_INFO.data_owner_name
    ++ ' '
    ++ *DATA_OBJ_INFO.data_owner_zone;
# XXX - ^^^
}


## RESOURCE ##

# RESOLVE HIERARCHY

# This rule is meant for project specific implementations where an project
# implementation is within an `on` block that restricts the resource resolution
# to entities relevant to the project.
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT, *OUT, *OPERATION, *HOST, *PARSER, *VOTE) {
# XXX - Because of https://github.com/irods/irods/issues/6463, an error
# happening in an `ON` condition needs to be captured and sent in the catch-all.
  if (errorcode(temporaryStorage.resource_resolve_hierarchy_err) == 0) {
    failmsg(-32000, temporaryStorage.resource_resolve_hierarchy_err);
  }
# XXX - ^^^
}
