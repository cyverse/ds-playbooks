# This rulebase contains the rules attached to Policy Execution Points for the 
# core CyVerse Data Store policies. All policy logic is in this file or 
# included by this file.
#
# © 2022 The Arizona Board of Regents on behalf of The University of Arizona. 
# For license information, see https://cyverse.org/license.

# The environment-specific configuration constants belong in the file 
# ipc-env.re.

@include 'ipc-env'

# All CyVerse specific, environment independent logic goes in the file
# ipc-logic.re. These rules will be called by the hooks implemented in
# ipc-custom.re. The rule names should be prefixed with 'ipc' and suffixed with
# the name of the rule hook that will call the custom rule.

# The shared logic usable by all CyVerse and third parties
@include 'json'
@include 'ipc-services'

@include 'ipc-logic'
@include 'ipc-repl'


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

# Common input types
#
# *Comm:
#   auth_scheme
#   client_addr
#   proxy_auth_info_auth_flag
#   proxy_auth_info_auth_scheme
#   proxy_auth_info_auth_str
#   proxy_auth_info_flag
#   proxy_auth_info_host
#   proxy_auth_info_ppid
#   proxy_rods_zone
#   proxy_sys_uid
#   proxy_user_name
#   proxy_user_other_info_user_comments
#   proxy_user_other_info_user_create
#   proxy_user_other_info_user_info
#   proxy_user_other_info_user_modify
#   proxy_user_type
#   user_auth_info_auth_flag
#   user_auth_info_auth_scheme
#   user_auth_info_auth_str
#   user_auth_info_flag
#   user_auth_info_host
#   user_auth_info_ppid
#   user_rods_zone
#   user_sys_uid
#   user_user_name
#   user_user_other_info_user_comments
#   user_user_other_info_user_create
#   user_user_other_info_user_info
#   user_user_other_info_user_modify
#   user_user_type
#
# *Instance: string
#
# *JSON_INPUT:
#   buf
#   len


# Tests to see if the given map contains the given key
#
#_ipc_hasKey : `KEYVALPAIR_MS_T` * string -> bool
_ipc_hasKey(*KVMap, *Key) = errorcode(*KVMap.'*Key') == 0


# Retrieves the value of the given key from the given map. If the key isn't 
# found, it returns the empty string.
#
#_ipc_getValue : `KEYVALPAIR_MS_T` * string -> string
_ipc_getValue(*KVMap, *Key) = if _ipc_hasKey(*KVMap, *Key) then *KVMap.'*Key' else ''


# Using an API PEP's data object operation input map, it determines if 
# data object(s) will need a checksum.
#
#_ipc_needsChecksum : `KEYVALPAIR_MS_T` -> bool
_ipc_needsChecksum(*DataObjOpInp) = 
  !_ipc_hasKey(*DataObjOpInp, 'regChksum') && !_ipc_hasKey(*DataObjOpInp, 'verifyChksum')


# N.B. This can be triggered by `iput -b -r`.
# N.B. `-k` adds `regChksum` to BulkOprInp.
# N.B. `-K` adds `verifyChksum` to BUKOPRINP.
# N.B. Overwriting a replica that has a checksum clears the checksum.
# N.B. `-X` handled transparently
# N.B. large files are not passed through rcBulkDataObjPut
#
# ALGORITHM:
#
# If neither *BulkOprInp.regChksum nor *BulkOprInp.verifyChksum exist, calculate the checksum of 
# replica on *BulkOprInp.resc_hier for each entry of *BulkOprInp.logical_path.
#
# *BulkOprInp: 
#   https://docs.irods.org/4.2.10/doxygen/group__data__object.html#gafeecbd87f6ba164e8c1d189c42a8c93e
#
# *BulkOprInpBBuf:
#   buf
#   len
#
pep_api_bulk_data_obj_put_post(*Instance, *Comm, *BulkOprInp, *BulkOprInpBBuf) {
  if (_ipc_needsChecksum(*BulkOprInp)) {
    foreach (*key in *BulkOprInp) {
      if (*key like 'logical_path_*') {
        ipc_ensureReplicasChecksum(
          _ipc_getValue(*BulkOprInp, *key), _ipc_getValue(*BulkOprInp, 'resc_hier') );
      }
    }
  }
}


# N.B. This isn't used by iCommands, but it is by the Java and Python APIs.
#
pep_api_data_obj_close_post(*Instance, *Comm, *DataObjCloseInp) {
  *msg = 'pep_api_data_obj_close_post(\*Instance, \*Comm, \*DataObjCloseInp) called\n'
    ++ '\t\*Instance = *Instance\n'
    ++ '\t\*Comm = *Comm\n'
    ++ '\t\*DataObjCloseInp = *DataObjCloseInp';

  writeLine('serverLog', *msg);
}


# N.B. This can be triggered by icp.
#
# ALGORITHM:
#
# If neither *DataObjCopyInp.regChksum nor *DataObjCopyInp.verifyChksum exist, calculate the
# checksum of *DataObjCopyInp.dst_obj_path on *DataObjCopyInp.dst_resc_hier.
#
# *DataObjCopyInp:
#   https://docs.irods.org/4.2.10/doxygen/group__data__object.html#gaad62fbc609d67726e15e7330bbbdf98d
#
# *TransStat: *NOT SUPPORTED*
#
pep_api_data_obj_copy_post(*Instance, *Comm, *DataObjCopyInp, *TransStat) {
  if (_ipc_needsChecksum(*DataObjCopyInp)) {
    *path = _ipc_getValue(*DataObjCopyInp, 'dst_obj_path');

    if (*path == '') {
      failmsg(
        -1, 'Could not determine path to created data object, (DataObjCopyInp = *DataObjCopyInp)' );
    } else {
      ipc_ensureReplicasChecksum(*path, _ipc_getValue(*DataObjCopyInp, 'dst_resc_hier'));
    }
  }
}


# N.B. This isn't used by iCommands, but it is by the Java and Python APIs.
#
pep_api_data_obj_create_post(*Instance, *Comm, *DataObjInp) {
  *msg = 'pep_api_data_obj_create_post(\*Instance, \*Comm, \*DataObjInp) called\n'
    ++ '\t\*Instance = *Instance\n'
    ++ '\t\*Comm = *Comm\n'
    ++ '\t\*DataObjInp = *DataObjInp';

  writeLine('serverLog', *msg);
}


# N.B. This isn't used by iCommands, but it is by the Java and Python APIs.
#
pep_api_data_obj_open_post(*Instance, *Comm, *DataObjInp) {
  *msg = 'pep_api_data_obj_open_post(\*Instance, \*Comm, \*DataObjInp) called\n'
    ++ '\t\*Instance = *Instance\n'
    ++ '\t\*Comm = *Comm\n'
    ++ '\t\*DataObjInp = *DataObjInp';

  writeLine('serverLog', *msg);
}


# N.B. This can be triggered by iput
#
# ALGORITHM:
#
# If neither *DataObjInp.regChksum nor *DataObjInp.verifyChksum exist, calculate the checksum of 
# *DataObjInp.obj_path on *DataObjInp.resc_hier.
#
# *DataObjInp:
#   https://docs.irods.org/4.2.10/doxygen/group__data__object.html#ga1b1d0d95bd1cbc6f07860d6f8174371f
#
# *DataObjInpBBuf:
#   buf
#   len
#
# *PORTAL_OPR_OUT: *NOT SUPPORTED*
#
pep_api_data_obj_put_post(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT) {
  if (_ipc_needsChecksum(*DataObjInp)) {
    *path = _ipc_getValue(*DataObjInp, 'obj_path');

    if (*path == '') {
      writeLine(
        'serverLog', 
        'Could not determine path to created data object, (DataObjInp = *DataObjInp)' );
    } else {
      ipc_ensureReplicasChecksum(*path, _ipc_getValue(*DataObjInp, 'resc_hier'));
    }
  }
}


# N.B. This can be triggered by ireg.
#
# ALGORITHM:
#
# If none of PhyPathRegInp.regRepl, PhyPathRegInp.regChksum, or PhyPathRegInp.verifyChksum are set,
# calculate the checksum of replica of PhyPathRegInp.obj_path on PhyPathRegInp.resc_hier.
#
# *PhyPathRegInp:
#   https://docs.irods.org/4.2.10/doxygen/group__data__object.html#gad63565afa69981320220cea5d1d1f6c3
#
pep_api_phy_path_reg_post(*Instance, *Comm, *PhyPathRegInp) {
  if (!_ipc_hasKey(*PhyPathRegInp, 'regRepl') && _ipc_needsChecksum(*PhyPathRegInp)) {
    *path = _ipc_getValue(*PhyPathRegInp, 'obj_path');

    if (*path == '') {
      writeLine(
        'serverLog', 
        'Could not determine path to created data object, (PhyPathRegInp = *PhyPathRegInp)' );
    } else {
      ipc_ensureReplicasChecksum(*path, _ipc_getValue(*PhyPathRegInp, 'resc_hier'));
    }
  }
}


# replica_open and replica_close work together.
# 
# N.B. These can be triggered by istream.
# N.B. Only `istream write` needs to be considered.
# N.B. This can create new, overwrite existing, modify existing, and append to existing data 
#      objects.
# N.B. This can target a specific resource, e.g., `istream write -R`, or existing replica, e.g., 
#      `istream write -n`.
# N.B. This can create a checksum, e.g., `istream write -k`. 
# N.B. When a data object with a checksum is overwritten, modified or appened to, the checksum is
#      cleared.
#
# ALGORITHM:
#
# When replica_open_post is called, if *DataObjInp.destRescName is defined, then store it and 
# *DataObjInp.obj_path in temporaryStorage. When replica_close_post is called, if destRescName and
# obj_path are in temporaryStorage, and *JSON_INPUT.buf.compute_checksum != true, compute the 
# checksum of obj_path.

# *DataObjInp: https://docs.irods.org/4.2.10/doxygen/structDataObjInp.html
#
# *JSON_OUTPUT : *NOT SUPPORTED*
# 
pep_api_replica_open_post(*Instance, *Comm, *DataObjInp, *JSON_OUTPUT) {
  *path = _ipc_getValue(*DataObjInp, 'obj_path');

  if (*path != '') {
    temporaryStorage.replica_dataObjPath = *path;
    temporaryStorage.replica_rescHier = _ipc_getValue(*DataObjInp, 'destRescName');
  }
}

# *SEE COMMON*
#
pep_api_replica_close_post(*Instance, *Comm, *JsonInput) {
  *path = _ipc_getValue(temporaryStorage, 'replica_dataObjPath');

  if (*path != '') {
    *chksumComputed = match json_deserialize(*JsonInput.buf) with
      | json_deserialize_val(*input, *_) => 
        match json_getValue(*input, 'compute_checksum') with
          | json_empty => false
          | json_bool(*v) => *v; 

    if (!*chksumComputed) {
      ipc_ensureReplicasChecksum(*path, _ipc_getValue(temporaryStorage, 'replica_rescHier'));
    }

    temporaryStorage.replica_dataObjPath = '';
    temporaryStorage.replica_rescHier = '';
  }
}


# N.B. This can be triggered by itouch.
#
# N.B. Only need to checksum something if it is a data object and it was created by itouch.
# N.B. itouch cannot be used to create a replica of an existing data object.
# If options.no_create = true, a data object wasn't created. 
# If options.replica_number or options.leaf_resource_name is set, a data object wasn't created.
#
# ALGORITHM:
#
# Check to see if JSON_INFUT.buf.options.no_create is false. If it is, check to see if neither 
# options.replica_number nor options.leaf_resource_name is set. If that's the case, check to see if
# the data object's 0 replica has a checksum. If it doesn't compute its checksum.
#
# TODO: test
#
# *SEE COMMON*
#
pep_api_touch_post(*Instance, *Comm, *JsonInput) {
  *input = match json_deserialize(*JsonInput.buf) with 
    | json_deserialize_val(*v, *_) => *v;

  *dataPath = match json_getValue(*input, 'logical_path') with
    | json_empty => ''
    | json_str(*s) => *s;

  if (*dataPath != '') {
    *options = json_getValue(*input, 'options');

    *noCreate = match json_getValue(*options, 'no_create') with
      | json_empty => false
      | json_bool(*b) => *b;

    *replNumSet = match json_getValue(*options, 'replica_number') with
      | json_empty => false
      | json_num(*n) => true;

    *rescNameSet = match json_getValue(*options, 'leaf_resource_name') with
      | json_empty => false
      | json_str(*_) => true;

    if (!*noCreate && !*replNumSet && !*rescNameSet) {
      msiSplitPath(*dataPath, *collPath, *dataName);

      foreach ( *rec in 
        SELECT DATA_CHECKSUM, DATA_RESC_HIER WHERE COLL_NAME = *collPath AND DATA_NAME = *dataName 
      ) {
        if (*rec.DATA_CHECKSUM == '') {
          ipc_ensureReplicasChecksum(*dataPath, *rec.DATA_RESC_HIER);
        }
      }
    }
  }
}


# N.B. These aren't used by iCommands or any official API, so let's not
# implement them.
#
pep_api_bulk_data_obj_reg_post(*Instance, *Comm, *BulkDataObjRegInp, *BULK_DATA_OBJ_REG_OUT) {}
pep_api_data_obj_create_and_stat_post(*Instance, *Comm, *DataObjInp, *OpenStat) {}


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
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT, *OUT, *OPERATION, *HOST, *PARSER, *VOTE) {}
