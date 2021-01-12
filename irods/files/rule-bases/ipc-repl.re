# Replication logic
# include this file from within ipc-custom.re
#
# Replication is controlled by AVUs attached to relevant root resources.
#
# ipc::hosted-collection COLL (forced|preferred)
#   When attached to a resource RESC, this AVU indicates that data objects that
#   belong to COLL are to have their primary replica stored on RESC. When the
#   unit is 'preferred', the user may override this. COLL is the path to the
#   base collection relative to the zone collection. If a resource is determined
#   by both the iRODS server a client connects to and an ipc::hosted-collection
#   AVU, the AVU takes precedence. If two or more AVUs match, the resource whose
#   COLL as the specific match is used.
#
# ipc::replica-resource REPL-RESC (forced|preferred)
#   When attached to a resource RESC, this AVU indicates that the resource
#   REPL-RESC is to asynchronously replicate the contents of RESC. When the unit
#   is 'preferred', the user may override this.
#
# DEPRECATED FUNCTIONALITY
#
# Unlike the logic in ipc-logic.re, the replication logic doesn't apply
# globally. Different projects may have different replication policies.
#
# THIRD PARTY REPLICATION LOGIC
#
# Third party replication logic goes in its own file, and the file should be
# included in ipc-custom.re. Third party logic should be implement a set of
# functions prefixed by the containing file name.
#
# Here's the list of functions that need to be provided by in the replication
# logic file.
#
# <file_name>_replBelongsTo(*Entity)
#   Determines if the provided collection or data object belongs to the project
#
#   Parameters:
#     Entity  the absolute iRODS path to the collection or data object
#
#   Return:
#     a Boolean indicating whether or not the collection or data object belongs
#     to the project
#
#   Example:
#     project_replBelongsTo : path -> boolean
#     project_replBelongsTo(*Entity) = str(*Entity) like '/' ++ ipc_ZONE ++ '/home/shared/project/*'
#
# <file_name>_replIngestResc
#   Returns the resource where newly ingested files for the project should be
#   stored.
#
#   Parameters:
#     none
#
#   Return:
#     a tuple where the first value is the name of the resource and the second
#     is a flag indicating whether or not this resource choice may be overridden
#     by the user.
#
#   Example:
#     project_replIngestResc : string * boolean
#     project_replIngestResc = ('projectIngestRes', true)
#
# <file_name>_replReplResc
#   Returns the resource where a replica of a data object should be stored.
#
#   Parameters:
#     none
#
#   Return:
#     a tuple where the first value is the name of the resource and the second
#     is a flag indicating whether or not this resource choice may be overridden
#     by the user.
#
#   Example:
#     project_replReplResc : string * boolean
#     project_replReplResc = ('projectReplRes', false)


# DEFERRED FUNCTIONS AND RULES

_replicate(*Object, *RescName) {
  _repl_logMsg('replicating *Object to *RescName');
  *err = errormsg(msiDataObjRepl(*Object, 'backupRescName=*RescName++++verifyChksum=', *status),
                  *msg);

  if (*err < 0 && *err != -808000 && *err != -817000) {
    _repl_logMsg('failed to replicate *Object, retry in 8 hours');
    _repl_logMsg(*msg);
# XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
#       iRODS version 4.2.1.
    delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_replicate(*Object, *RescName)}
  } else {
    _repl_logMsg('replicated *Object');
  }
}


# DEPRECATED
_mvReplicas(*DataPath, *IngestResc, *ReplResc) {
  _repl_logMsg('moving replicas of *DataPath');

  *replFail = false;
  (*ingestName, *ingestOptional) = *IngestResc;
  (*replName, *replOptional) = *ReplResc;

  if (_replicate(*DataPath, *ingestName) < 0) {
    *replFail = true;
  }

  if (_replicate(*DataPath, *replName) < 0) {
    *replFail = true;
  }

  if (*replFail) {
    fail;
  }

  # Once a replica exists on all the project's resource, remove the other replicas
  msiSplitPath(*DataPath, *collPath, *dataName);

  foreach (*repl in SELECT DATA_RESC_NAME WHERE COLL_NAME = '*collPath' AND DATA_NAME = '*dataName')
  {
    *rescName = *repl.DATA_RESC_NAME;

    if (*rescName != *ingestName && *rescName != *replName) {
      msiDataObjTrim(*DataPath, *rescName, 'null', '1', 'null', *status);
    }
  }
}


_repl_mvReplicas(*DataPath, *IngestName, *ReplName) {
  _repl_logMsg('moving replicas of *DataPath');

  *replFail = false;

  if (_replicate(*DataPath, *IngestName) < 0) {
    *replFail = true;
  }

  if (_replicate(*DataPath, *ReplName) < 0) {
    *replFail = true;
  }

  if (*replFail) {
    fail;
  }

  # Once a replica exists on all the project's resource, remove the other replicas
  msiSplitPath(*DataPath, *collPath, *dataName);

  foreach (*repl in SELECT DATA_RESC_NAME WHERE COLL_NAME = '*collPath' AND DATA_NAME = '*dataName')
  {
    *rescName = *repl.DATA_RESC_NAME;

    if (*rescName != *IngestName && *rescName != *ReplName) {
      msiDataObjTrim(*DataPath, *rescName, 'null', '1', 'null', *status);
    }
  }
}


_syncReplicas(*Object) {
  _repl_logMsg('syncing replicas of *Object');
  *err = errormsg(msiDataObjRepl(*Object, 'all=++++updateRepl=++++verifyChksum=', *status), *msg);

  if (*err < 0 && *err != -808000) {
    _repl_logMsg('failed to sync replicas of *Object trying again in 8 hours');
    _repl_logMsg(*msg);
# XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
#       iRODS version 4.2.1.
    delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_syncReplicas(*Object)}
  } else {
    _repl_logMsg('synced replicas of *Object');
  }
}


# SUPPORTING FUNCTIONS AND RULES

_defaultIngestResc : string * boolean
_defaultIngestResc = (ipc_DEFAULT_RESC, true)


_defaultReplResc : string * boolean
_defaultReplResc = (ipc_DEFAULT_REPL_RESC, true)


_delayTime : int
_delayTime = if (errorcode(temporaryStorage.delayTime) < 0) {
               temporaryStorage.delayTime = '1';
               1;
             } else {
               int(temporaryStorage.delayTime);
             }


_incDelayTime {
  temporaryStorage.delayTime = str(1 + int(temporaryStorage.delayTime));
}


_repl_logMsg(*Msg) {
  writeLine('serverLog', 'DS: *Msg');
}


# DEPRECATED
# XXX - As of 4.2.1, Booleans and tuples are not supported by packing instructions. The resource
#       description tuple must be expanded, and the second term needs to be converted to a string.
#       See https://github.com/irods/irods/issues/3634 for Boolean support.
# TODO - verify that this is still the case in iRODS 4.2.2.
_scheduleMv(*Object, *IngestName, *IngestOptionalStr, *ReplName, *ReplOptionalStr) {
  delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>8h REPEAT UNTIL SUCCESS</EF>')
  {_mvReplicas(*Object, (*IngestName, bool(*IngestOptionalStr)), (*ReplName, bool(*ReplOptionalStr)))}

  _incDelayTime;
}


_repl_scheduleMv(*Object, *IngestName, *ReplName) {
  delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>8h REPEAT UNTIL SUCCESS</EF>')
  {_repl_mvReplicas(*Object, *IngestName, *ReplName)}

  _incDelayTime;
}


# DEPRECATED
_scheduleMoves(*Entity, *IngestResc, *ReplResc) {
  (*ingestName, *ingestOptional) = *IngestResc;
  (*replName, *replOptional) = *ReplResc;

  msiGetObjType(*Entity, *type);

  if (*type == '-c') {
    # if the entity is a collection
    foreach (*collPat in list(*Entity, *Entity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collPath = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;
        _scheduleMv('*collPath/*dataName',
                    *ingestName, str(*ingestOptional),
                    *replName, str(*replOptional));
      }
    }
  } else if (*type == '-d') {
    # if the entity is a data object
    _scheduleMv(*Entity, *ingestName, str(*ingestOptional), *replName, str(*replOptional));
  }
}


_repl_scheduleMoves(*Entity, *IngestName, *ReplName) {
  msiGetObjType(*Entity, *type);

  if (*type == '-c') {
    # if the entity is a collection
    foreach (*collPat in list(*Entity, *Entity ++ '/%')) {
      foreach (*obj in SELECT COLL_NAME, DATA_NAME WHERE COLL_NAME LIKE '*collPat') {
        *collPath = *obj.COLL_NAME;
        *dataName = *obj.DATA_NAME;
        _repl_scheduleMv('*collPath/*dataName', *IngestName, *ReplName);
      }
    }
  } else if (*type == '-d') {
    # if the entity is a data object
    _repl_scheduleMv(*Entity, *IngestName, *ReplName);
  }
}


_scheduleRepl(*Object, *RescName) {
  delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>1s REPEAT 0 TIMES</EF>')
  {_replicate(*Object, *RescName)}

  _incDelayTime;
}


_scheduleSyncReplicas(*Object) {
  msiSplitPath(*Object, *collPath, *dataName);
  foreach (*cnt in SELECT COUNT(DATA_REPL_NUM)
                   WHERE COLL_NAME = '*collPath'
                     AND DATA_NAME = '*dataName'
                     AND DATA_REPL_STATUS = '0') {
    if (int(*cnt.DATA_REPL_NUM) > 0) {
      delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>1s REPEAT 0 TIMES</EF>')
      { # _syncReplicas(*Object);
# XXX - Due to https://github.com/irods/irods/issues/3621, _syncReplicas has been inlined. Verify
#       that this is still the case in iRODS 4.2.2.
        # _repl_logMsg('syncing replicas of *Object');
  # XXX - Due to https://github.com/irods/irods/issues/3621, _repl_logMsg has been inlined. Verify
  #       that this is still the case in iRODS 4.2.2.
        writeLine('serverLog', 'DS: syncing replicas of *Object');

        *err = errormsg(msiDataObjRepl(*Object, 'all=++++updateRepl=++++verifyChksum=', *status),
                        *msg);

        if (*err < 0 && *err != -808000) {
          _repl_logMsg('failed to sync replicas of *Object trying again in 8 hours');
          _repl_logMsg(*msg);
  # XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
  #       iRODS version 4.2.1.
          delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_syncReplicas(*Object)}
        } else {
          # _repl_logMsg('synced replicas of *Object');
  # XXX - Due to https://github.com/irods/irods/issues/3621, _repl_logMsg has been inlined. Verify
  #       that this is still the case in iRODS 4.2.2.
          writeLine('serverLog', 'DS: synced replicas of *Object');
        }
# XXX - ^^^
      }

      _incDelayTime;
    }
  }
}


# DEPRECATED
_createOrOverwrite(*Object, *IngestResc, *ReplResc) {
  if ($writeFlag == 0) {
    (*ingestName, *optional) = *IngestResc;
    (*replName, *optional) = *ReplResc;
# XXX - Async Automatic replication is too slow and plugs up the rule queue at the moment
#    _scheduleRepl(*Object, if $rescName == *replName then *ingestName else *replName);
# XXX - ^^^
  } else {
    _scheduleSyncReplicas(*Object);
  }
}


_repl_createOrOverwrite(*Object, *IngestResc, *ReplResc) {
  if ($writeFlag == 0) {
# XXX - Async Automatic replication is too slow and plugs up the rule queue at the moment
#    _scheduleRepl(*Object, if $rescName == *ReplResc then *IngestResc else *ReplResc);
# XXX - ^^^
  } else {
    _scheduleSyncReplicas(*Object);
  }
}


_setDefaultResc(*Resource) {
  (*name, *optional) = *Resource;
  msiSetDefaultResc(*name, if *optional then 'preferred' else 'forced');
}


# Given an absolute path to a collection, this rule determines the resource
# where member data objects have their primary replicas stored. It returns a
# two-tuple with the first is element is the name of the resource, and the
# second is the value 'forced' or 'preferred'. 'forced' means that the user
# cannot override this choice, and 'preferred' means they can.
_repl_findResc(*DataPath) {
  msiSplitPath(*DataPath, *collPath, *dataName);
  *resc = ipc_DEFAULT_RESC;
  *residency = 'preferred';
  *bestColl = '/';

  foreach (*record in SELECT META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS, RESC_NAME
                      WHERE META_RESC_ATTR_NAME = 'ipc::hosted-collection') {
    if (*collPath ++ '/' like '/' ++ ipc_ZONE ++ '/' ++ *record.META_RESC_ATTR_VALUE ++ '/*') {
      if (strlen(*record.META_RESC_ATTR_VALUE) > strlen(*bestColl)) {
        *resc = *record.RESC_NAME;
        *residency = *record.META_RESC_ATTR_UNITS;
        *bestColl = *record.META_RESC_ATTR_VALUE;
      }
    }
  }

  *result = (*resc, *residency);
  *result;
}


# Given a resource, this rule determines the list of resources that
# asynchronously replicate its replicas.
_repl_findReplResc(*Resc) {
  *repl = ipc_DEFAULT_REPL_RESC;
  *residency = 'preferred';

  foreach (*record in SELECT META_RESC_ATTR_VALUE, META_RESC_ATTR_UNITS
                      WHERE RESC_NAME = *Resc AND META_RESC_ATTR_NAME = 'ipc::replica-resource') {
    *repl = *record.META_RESC_ATTR_VALUE;
    *residency = *record.META_RESC_ATTR_UNITS;
  }

  *result =(*repl, *residency);
  *result;
}


# REPLICATION RULES

# This rule ensures that copies of files are replicated.

# DEPRECATED
_old_replCopy {
  on (aegis_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, aegis_replIngestResc, aegis_replReplResc);
  }
  on (avra_replBelongsTo(/$objPath)) {
  }
  on (de_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, de_replIngestResc, de_replReplResc);
  }
  on (pire_replBelongsTo(/$objPath)) {
  }
  on (terraref_replBelongsTo(/$objPath)) {
  }
}
_old_replCopy {
  _createOrOverwrite($objPath, _defaultIngestResc, _defaultReplResc);
}

# TODO - Once deprecated functionality is gone, move common logic with replPut into
#        _repl_createOrOverwrite.
replCopy {
  (*resc, *_) = _repl_findResc($objPath);

  if (*resc != ipc_DEFAULT_RESC) {
    (*repl, *_) = _repl_findReplResc(*resc);
    _repl_createOrOverwrite($objPath, *resc, *repl);
  } else {
    _old_replCopy;
  }
}


# This rule updates the replicas if needed after a collection or data object has
# been moved
#
# Parameters:
#  SourceObject  the absolute path to the collection or data object before it
#                was moved
#  DestObject    the absolute path after it was moved

# DEPRECATED
_old_replEntityRename(*SourceObject, *DestObject) {
  on (aegis_replBelongsTo(/*DestObject)) {
    if (!aegis_replBelongsTo(/*SourceObject)) {
      _scheduleMoves(*DestObject, aegis_replIngestResc, aegis_replReplResc);
    }
  }
  on (avra_replBelongsTo(/*DestObject)) {
    if (!avra_replBelongsTo(/*SourceObject)) {
      _scheduleMoves(*DestObject, avra_replIngestResc, avra_replReplResc);
    }
  }
  on (de_replBelongsTo(/*DestObject)) {
  }
  on (pire_replBelongsTo(/*DestObject)) {
    if (!pire_replBelongsTo(/*SourceObject)) {
      _scheduleMoves(*DestObject, pire_replIngestResc, pire_replReplResc);
    }
  }
  on (terraref_replBelongsTo(/*DestObject)) {
    if (!terraref_replBelongsTo(/*SourceObject)) {
      _scheduleMoves(*DestObject, terraref_replIngestResc, terraref_replReplResc);
    }
  }
}
_old_replEntityRename(*SourceObject, *DestObject) {
  on (aegis_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
  on (avra_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
  on (pire_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
  on (terraref_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
}
# DEPRECATION NOTE: When the conditional versions are ready to be deleted, merge this into
#                   replEntityRename.
_old_replEntityRename(*SourceObject, *DestObject) {
  writeLine('serverLog', '_old_replEntityRename(*SourceObject, *DestObject) {');
  (*srcResc, *_) = _repl_findResc(*SourceObject);

  if (*srcResc != ipc_DEFAULT_RESC) {
    _repl_scheduleMoves(*DestObject, ipc_DEFAULT_RESC, ipc_DEFAULT_REPL_RESC);
  }
}

replEntityRename(*SourceObject, *DestObject) {
  (*destResc, *_) = _repl_findResc(*DestObject);

  if (*destResc != ipc_DEFAULT_RESC) {
    (*srcResc, *_) = _repl_findResc(*SourceObject);

    if (*srcResc != *destResc) {
      (*destRepl, *_) = _repl_findReplResc(*destResc);
      _repl_scheduleMoves(*DestObject, *destResc, *destRepl);
    }
  } else {
    _old_replEntityRename(*SourceObject, *DestObject);
  }
}


# This rule ensures that registered files are replicated.
#
replFilePathReg {
  on (aegis_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, aegis_replIngestResc, aegis_replReplResc);
  }
  on (avra_replBelongsTo(/$objPath)) {
  }
  on (de_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, de_replIngestResc, de_replReplResc);
  }
  on (pire_replBelongsTo(/$objPath)) {
  }
  on (terraref_replBelongsTo(/$objPath)) {
  }
}
replFilePathReg {
  _createOrOverwrite($objPath, _defaultIngestResc, _defaultReplResc);
}


# This rule ensures that uploaded files are replicated.

# DEPRECATED
_old_replPut {
  on (aegis_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, aegis_replIngestResc, aegis_replReplResc);
  }
  on (avra_replBelongsTo(/$objPath)) {
  }
  on (de_replBelongsTo(/$objPath)) {
    _createOrOverwrite($objPath, de_replIngestResc, de_replReplResc);
  }
  on (pire_replBelongsTo(/$objPath)) {
  }
  on (terraref_replBelongsTo(/$objPath)) {
  }
}
_old_replPut {
  _createOrOverwrite($objPath, _defaultIngestResc, _defaultReplResc);
}

replPut {
  (*resc, *_) = _repl_findResc($objPath);

  if (*resc != ipc_DEFAULT_RESC) {
    (*repl, *_) = _repl_findReplResc(*resc);
    _repl_createOrOverwrite($objPath, *resc, *repl);
  } else {
    _old_replPut;
  }
}


# This rule ensures that the correct resource is chosen for first replica of a
# newly created data object.

# DEPRECATED
_old_replSetRescSchemeForCreate {
  on (aegis_replBelongsTo(/$objPath)) {
    _setDefaultResc(aegis_replIngestResc);
  }
  on (avra_replBelongsTo(/$objPath)) {
    _setDefaultResc(avra_replIngestResc);
  }
  on (de_replBelongsTo(/$objPath)) {
    _setDefaultResc(de_replIngestResc);
  }
  on (pire_replBelongsTo(/$objPath)) {
    _setDefaultResc(pire_replIngestResc);
  }
  on (terraref_replBelongsTo(/$objPath)) {
    _setDefaultResc(terraref_replIngestResc);
  }
}
_old_replSetRescSchemeForCreate {
  _setDefaultResc(_defaultIngestResc);
}

replSetRescSchemeForCreate {
  (*resc, *residency) = _repl_findResc($objPath);

  if (*resc != ipc_DEFAULT_RESC) {
    msiSetDefaultResc(*resc, *residency);
  } else {
    _old_replSetRescSchemeForCreate;
  }
}


# This rule ensures that the correct resource is chosen for the second and
# subsequent replicas of a data object.

# DEPRECATED
_old_replSetRescSchemeForRepl {
  on (aegis_replBelongsTo(/$objPath)) {
    _setDefaultResc(aegis_replReplResc);
  }
  on (avra_replBelongsTo(/$objPath)) {
    _setDefaultResc(avra_replReplResc);
  }
  on (de_replBelongsTo(/$objPath)) {
    _setDefaultResc(de_replReplResc);
  }
  on (pire_replBelongsTo(/$objPath)) {
    _setDefaultResc(pire_replReplResc);
  }
  on (terraref_replBelongsTo(/$objPath)) {
    _setDefaultResc(terraref_replReplResc);
  }
}
_old_replSetRescSchemeForRepl {
  _setDefaultResc(_defaultReplResc);
}

replSetRescSchemeForRepl {
  (*resc, *_) = _repl_findResc($objPath);

  if (*resc != ipc_DEFAULT_RESC) {
    (*repl, *residency) = _repl_findReplResc(*resc);
    msiSetDefaultResc(*repl, *residency);
  } else {
    _old_replSetRescSchemeForRepl;
  }
}
