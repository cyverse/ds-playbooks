# DE project policy
# include this file from with ipc-custom.re

_de_STAGING_BASE = '/' ++ ipc_ZONE ++ '/jobs'


_de_inStagedJob(*Path) = *Path like regex '^' ++ _de_STAGING_BASE ++ '/[^/]+/.+'


_de_inStaging(*Path) = str(*Path) like _de_STAGING_BASE ++ '/*'


_de_getJobInfo(*StagingRelPath) =
  let *info.id = elem(split(*StagingRelPath, '/'), 0) in
  let *info.creator = '' in
  let *info.appId = '' in
  let *info.archiveBase = '' in
  let *stagingBase = _de_STAGING_BASE ++ '/' ++ *info.id in
  let *_ = foreach(*res in select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE
                           where COLL_NAME = *stagingBase) {
             if (*res.META_COLL_ATTR_NAME == 'ipc-creator') {
               *info.creator = *res.META_COLL_ATTR_VALUE;
             } else if (*res.META_COLL_ATTR_NAME == 'ipc-real-output') {
               *info.archiveBase = *res.META_COLL_ATTR_VALUE;
             } else if (*res.META_COLL_ATTR_NAME == 'ipc-analysis-id') {
               *info.appId = *res.META_COLL_ATTR_VALUE;
             }
           } in
  *info


_de_createArchiveColl(*ArchiveColl, *StageColl, *Creator, *AppId, *JobId) {
  *clientArg = execCmdArg(*Creator);
  *stageArg = execCmdArg(*StageColl);
  *archiveArg = execCmdArg(*ArchiveColl);
  *execArg = execCmdArg(*JobId);
  *appArg = execCmdArg(*AppId);
  *argStr = '*clientArg *stageArg *archiveArg *execArg *appArg';

  *status = errormsg(msiExecCmd('de-create-collection', *argStr, 'null', 'null', 'null', *out),
                     *msg);

  if (*status < 0) {
    writeLine('serverLog', 'DE: Failed to create archive collection: *msg');
    msiGetStderrInExecCmdOut(*out, *errMsg);
    writeLine('serverLog', 'DE: *errMsg');
    cut;
    failmsg(*status, *errMsg);
  }
}


_de_archiveData(*StagingPath) {
  *stagingRelPath = triml(*StagingPath, _de_STAGING_BASE ++ '/');
  *jobInfo = _de_getJobInfo(*stagingRelPath);

  if (*jobInfo.creator != '' && *jobInfo.archiveBase != '') {
    if (*stagingRelPath like regex '^' ++ *jobInfo.id ++ '/[^/]+') {
      *jobStagingBase = _de_STAGING_BASE ++ '/' ++ *jobInfo.id;

      _de_createArchiveColl(*jobInfo.archiveBase, *jobStagingBase, *jobInfo.creator,
                            *jobInfo.appId, *jobInfo.id);
    }

    *archiveObj = *jobInfo.archiveBase ++ '/' ++ triml(*stagingRelPath, *jobInfo.id ++ '/');
    *clientArg = execCmdArg(*jobInfo.creator);
    *stageArg = execCmdArg(*StagingPath);
    *archiveArg = execCmdArg(*archiveObj);
    *execArg = execCmdArg(*jobInfo.id);
    *appArg = execCmdArg(*jobInfo.appId);
    *argStr = '*clientArg *stageArg *archiveArg *execArg *appArg';
    *status = errormsg(msiExecCmd('de-archive-data', *argStr, 'null', 'null', 'null', *out), *msg);

    if (*status < 0) {
      writeLine('serverLog', 'DE: Failed to archive data object: *msg');
      msiGetStderrInExecCmdOut(*out, *errMsg);
      writeLine('serverLog', 'DE: *errMsg');
      cut;
      failmsg(*status, *errMsg);
    }
  }
}


_de_createArchiveCollFor(*StagingColl) {
  if (_de_inStagedJob(*StagingColl)) {
    *stagingRelPath = triml(*StagingColl, _de_STAGING_BASE ++ '/');
    *jobInfo = _de_getJobInfo(*stagingRelPath);

    if (*jobInfo.creator != '' && *jobInfo.archiveBase != '') {
      if (*stagingRelPath like regex '^' ++ *jobInfo.id ++ '/[^/]+') {
        *jobStagingBase = _de_STAGING_BASE ++ '/' ++ *jobInfo.id;

        _de_createArchiveColl(*jobInfo.archiveBase, *jobStagingBase, *jobInfo.creator,
                              *jobInfo.appId, *jobInfo.id);
      }

      *archiveColl = *jobInfo.archiveBase ++ '/' ++ triml(*stagingRelPath, *jobInfo.id ++ '/');

      _de_createArchiveColl(*archiveColl, *StagingColl, *jobInfo.creator, *jobInfo.appId,
                            *jobInfo.id);
    }
  }
}


# Determines if the provided collection or data object is in the DE staging area
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the staging area, otherwise
#  false
#
de_replBelongsTo : path -> boolean
de_replBelongsTo(*Entity) = _de_inStaging(*Entity)


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
de_replIngestResc : string * boolean
de_replIngestResc = (ipc_DEFAULT_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
de_replReplResc : string * boolean
de_replReplResc = de_replIngestResc


# Verifies that a collection or data object isn't being moved within the staging
#  area.
de_acPreProcForObjRename(*SourceObject, *DestObject) {
  if (_de_inStaging(*SourceObject) || _de_inStaging(*DestObject)) {
    cut;
    failmsg(-350000, "CYVERSE ERROR:  attempt to move entity within DE's staging area");
  }
}


exclusive_acCreateCollByAdmin(*ParColl, *ChildColl) {
  on (_de_inStaging(*ParColl/*ChildColl)) {
    _de_createArchiveCollFor(*ParColl/*ChildColl);
  }
}


exclusive_acPostProcForCollCreate {
  on (_de_inStaging($collName)) {
    _de_createArchiveCollFor($collName);
  }
}


exclusive_acPostProcForCopy {
  on (_de_inStagedJob($objPath)) {
    _de_archiveData($objPath);
  }
}


exclusive_acPostProcForPut {
  on (_de_inStagedJob($objPath)) {
    _de_archiveData($objPath);
  }
}


pep_resource_resolve_hierarchy_pre(*OUT) {
  on ($KVPairs.logical_path like regex '^' ++ _de_STAGING_BASE ++ '/[^/]+') {
    cut;
    failmsg(-350000, "CYVERSE ERROR:  cannot put files into " ++ _de_STAGING_BASE);
  }
}
