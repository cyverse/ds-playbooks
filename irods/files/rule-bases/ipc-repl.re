# Replication logic
# include this file from within ipc-custom.re
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
  *err = errormsg(msiDataObjRepl(*Object, 'backupRescName=*RescName++++verifyChksum=', *status),
                  *msg);

  if (*err < 0 && *err != -808000 && *err != -817000) {
    writeLine('serverLog', *msg);
# XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
#       iRODS version 4.2.1.
    delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_replicate(*Object, *RescName);}
  }
}


_mvReplicas(*DataPath, *IngestResc, *ReplResc) {
  *replFail = false;
  (*ingestName, *ingestOptional) = *IngestResc;

  if (!*ingestOptional) {
    if (_replicate(*DataPath, *ingestName) < 0) {
      *replFail = true;
    }
  }

# XXX - Added a semicolon to prevent irodsReServer failure. See
#       https://github.com/irods/irods/issues/3629. Verify that this is still the case in iRODS
#       4.2.2.
  ;

  (*replName, *replOptional) = *ReplResc;

  if (!*replOptional) {
    if (_replicate(*DataPath, *replName) < 0) {
      *replFail = true;
    }

    if (*replFail) { fail; }

    # Once a replica exists on all the project's resource, remove the other replicas
    msiSplitPath(*DataPath, *collPath, *dataName);

    foreach (*repl in SELECT DATA_RESC_NAME
                      WHERE COLL_NAME = '*collPath' AND DATA_NAME = '*dataName') {
      *rescName = *repl.DATA_RESC_NAME;

      if (*rescName != *ingestName && *rescName != *replName) {
        msiDataObjTrim(*DataPath, *rescName, 'null', '1', 'null', *status);
      }
    }
  }
}


_syncReplicas(*Object) {
  *err = errormsg(msiDataObjRepl(*Object, 'all=++++updateRepl=++++verifyChksum=', *status), *msg);

  if (*err < 0 && *err != -808000) {
    writeLine('serverLog', *msg);
# XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
#       iRODS version 4.2.1.
    delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_syncReplicas(*Object);}
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


# As of 4.2.1, Booleans and tuples are not supported by packing instructions. The resource
# description tuple must be expanded, and the second term needs to be converted to a string.
# TODO verify that this is still the case in iRODS 5. See https://github.com/irods/irods/issues/3634
# for Boolean support.
_scheduleMv(*Object, *IngestName, *IngestOptionalStr, *ReplName, *ReplOptionalStr) {
  delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>8h REPEAT UNTIL SUCCESS</EF>') 
  {_mvReplicas(*Object, (*IngestName, bool(*IngestOptionalStr)), (*ReplName, bool(*ReplOptionalStr)));}

  _incDelayTime;
}


_scheduleMoves(*Entity, *IngestResc, *ReplResc) {
  (*ingestName, *ingestOptional) = *IngestResc;
  (*replName, *replOptional) = *ReplResc;

  if (!*ingestOptional || !*replOptional) {
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
}


_scheduleRepl(*Object, *RescName) {
  delay('<PLUSET>' ++ str(_delayTime) ++ 's</PLUSET><EF>1s REPEAT 0 TIMES</EF>') 
  {_replicate(*Object, *RescName);}

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

        *err = errormsg(msiDataObjRepl(*Object, 'all=++++updateRepl=++++verifyChksum=', *status),
                        *msg);

        if (*err < 0 && *err != -808000) {
          writeLine('serverLog', *msg);
  # XXX - Nesting delay rules is a hack to work around `delay` ignoring `succeed`.  This is fixed in
  #       iRODS version 4.2.1.
          delay('<PLUSET>8h</PLUSET><EF>1s REPEAT 0 TIMES</EF>') {_syncReplicas(*Object);}
        }
# XXX - ^^^
      }
      _incDelayTime;
    }
  }
}


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


_setDefaultResc(*Resource) {
  (*name, *optional) = *Resource;
  msiSetDefaultResc(*name, if *optional then 'preferred' else 'forced');
}


# REPLICATION RULES

# This rule ensures that copies of files are replicated.
#
replCopy {
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
}
replCopy {
  _createOrOverwrite($objPath, _defaultIngestResc, _defaultReplResc);
 }


# This rule updates the replicas if needed after a collection or data object has
# been moved
#
# Parameters:
#  SourceObject  the absolute path to the collection or data object before it
#                was moved
#  DestObject    the absolute path after it was moved
#
replEntityRename(*SourceObject, *DestObject) {
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
}
replEntityRename(*SourceObject, *DestObject) {
  if (aegis_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
  if (avra_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
  if (pire_replBelongsTo(/*SourceObject)) {
    _scheduleMoves(*DestObject, _defaultIngestResc, _defaultReplResc);
  }
}


# This rule ensures that uploaded files are replicated.
#
replPut {
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
}
replPut {
  _createOrOverwrite($objPath, _defaultIngestResc, _defaultReplResc);
}


# This rule ensures that the correct resource is chosen for first replica of a
# newly created data object.
#
replSetRescSchemeForCreate {
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
}
replSetRescSchemeForCreate {
  _setDefaultResc(_defaultIngestResc);
}


# This rule ensures that the correct resource is chosen for the second and
# subsequent replicas of a data object.
#
replSetRescSchemeForRepl {
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
}
replSetRescSchemeForRepl {
  _setDefaultResc(_defaultReplResc);
}
