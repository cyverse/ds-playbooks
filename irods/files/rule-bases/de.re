# DE project policy
# include this file from with ipc-custom.re

_de_STAGING_BASE = '/' ++ ipc_ZONE ++ '/jobs'


_de_inStaging(*Object) = *Object like regex '^' ++ _de_STAGING_BASE ++ '/[^/]+/.+'


_de_createArchiveColl(*Coll, *Creator, *AppId, *JobId) {
  *clientArg = execCmdArg(*Creator);
  *collArg = execCmdArg(*Coll);
  *execArg = execCmdArg(*JobId);
  *appArg = execCmdArg(*AppId);
  *argStr = '*clientArg *collArg *execArg *appArg';

  *status = errormsg(msiExecCmd('de-create-collection', *argStr, 'null', 'null', 'null', *out),
                     *msg);

  if (*status < 0) {
    writeLine('serverLog', 'DE: Failed to create archive collection: *msg');
    msiGetStderrInExecCmdOut(*out, *errMsg);
    writeLine('serverLog', 'DE: *errMsg');
  }
}


_de_createArchiveCollFor(*StagingColl) {
  *stagingRelPath = triml(*StagingColl, _de_STAGING_BASE ++ '/');
  *jobId = elem(split(*stagingRelPath, '/'), 0);
  *jobStagingBase = _de_STAGING_BASE ++ '/*jobId';
  *creator = '';
  *jobArchiveBase = '';
  *appId = '';
  *query = select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE where COLL_NAME = '*jobStagingBase';

  foreach (*res in *query) {
    if (*res.META_COLL_ATTR_NAME == 'ipc-creator') {
      *creator = *res.META_COLL_ATTR_VALUE;
    } else if (*res.META_COLL_ATTR_NAME == 'ipc-real-output') {
      *jobArchiveBase = *res.META_COLL_ATTR_VALUE;
    } else if (*res.META_COLL_ATTR_NAME == 'ipc-analysis-id') {
      *appId = *res.META_COLL_ATTR_VALUE;
    }
  }

  if (*creator != '' && *jobArchiveBase != '') {
    if (*stagingRelPath like regex '^*jobId/[^/]+$') {
      _de_createArchiveColl(*jobArchiveBase, *creator, *appId, *jobId);
    }

    *archiveColl = '*jobArchiveBase/' ++ triml(*stagingRelPath, '*jobId/');
    _de_createArchiveColl(*archiveColl, *creator, *appId, *jobId);
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
de_replBelongsTo(*Entity) = str(*Entity) like _de_STAGING_BASE ++ '/*'


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


de_acPostProcForCollCreate {
  if (_de_inStaging($collName)) {
    _de_createArchiveCollFor($collName);
  }
}


de_acPostProcForObjRename(*SourceObject, *DestObject) {
  if (_de_inStaging(*DestObject)) {
    msiGetObjType(*DestObject, *type);

    if (*type == '-c') {
      _de_createArchiveCollFor(*DestObject);
    }
  }
}
