# DE project policy
# include this file from with ipc-custom.re

_de_JOBS_BASE = '/' ++ ipc_ZONE ++ '/jobs'


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
de_replBelongsTo(*Entity) = str(*Entity) like _de_JOBS_BASE ++ '/*'


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
  if ($collName like regex '^' ++ _de_JOBS_BASE ++ '/[^/]+/.+') {
    *stagingRelPath = triml($collName, _de_JOBS_BASE ++ '/');
    *jobId = elem(split(*stagingRelPath, '/'), 0);
    *jobStagingBase = _de_JOBS_BASE ++ '/*jobId';
    *creator = '';
    *jobArchiveBase = '';
    *appId = '';
    *query = select META_COLL_ATTR_NAME, META_COLL_ATTR_VALUE where COLL_NAME = '*jobStagingBase';

    foreach (*res in *query) {
      if (*res.META_COLL_ATTR_NAME == 'ipc-creator') {
        *creator = *res.META_COLL_ATTR_VALUE;
      } else if (res.META_COLL_ATTR_NAME == 'ipc-real-output') {
        *jobArchiveBase = *res.META_COLL_ATTR_VALUE;
      } else if (*res.META_COLL_ATTR_NAME == 'ipc-analysis-id') {
        *appId = *res.META_COLL_ATTR_VALUE;
      }
    }

    if (*creator != '' && *jobArchiveColl != '') {
      *archivePath = '*jobArchiveBase/' ++ triml(*stagingRelPath, '*jobId/');
      *clientArg = execCmdArg(*Creator);
      *execArg = execCmdArg(*jobId);
      *appArg = execCmdArg(*appId);

      if ($collName like regex '^*jobStagingBase/[^/]+$') {
        *baseCollArg = execCmdArg(*jobArchiveBase);
        *baseArgs = "*clientArg *baseCollArg *execArg *appArg";
        msiExecCmd("de-create-collection", *baseArgs, "null", "null", "null", *out);
      }

      *collArg = execCmdArg(*archivePath);
      *args = "*clientArg *collArg *execArg *appArg";
      msiExecCmd("de-create-collection", *args, "null", "null", "null", *out);
    }
  }
}


# TODO implement
de_acPostProcForObjRename(*SourceObject, *DestObject) {}
