# PIRE project policy
# include this file from within ipc-custom.re

@include 'pire-env'


_pire_isForPIRE(*Path) = str(*Path) like str(pire_BASE_COLL) ++ '/*'


# Determines if the provided collection or data object belongs to the PIRE
# project
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the project, otherwise false
#
pire_replBelongsTo : path -> boolean
pire_replBelongsTo(*Entity) = _pire_isForPIRE(*Entity)


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
pire_replIngestResc : string * boolean
pire_replIngestResc = (pire_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
pire_replReplResc : string * boolean
pire_replReplResc = pire_replIngestResc


# Restrict the PIRE resource to files in the PIRE collection
pep_resource_resolve_hierarchy_pre(*OUT) {
  on (pire_RESC != ipc_DEFAULT_RESC
      && $KVPairs.resc_hier == pire_RESC
      && !_pire_isForPIRE($KVPairs.logical_path)) {
    *msg = 'CYVERSE ERROR:  ' ++ pire_RESC ++ ' usage is limited to the EHT collection, '
           ++ str(pire_BASE_COLL);

    failmsg(-32000, *msg);
  }
}
