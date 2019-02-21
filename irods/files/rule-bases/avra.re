# Avra project policy
# include this file from within ipc-custom.re

@include 'avra-env'

_avra_isForAvra(*Path) =
  let *strName = str(avra_BASE_COLL) in
  *strName != '' && str(*Path) like *strName ++ '/*'


# Determines if the provided collection or data object belongs to the Avra
# project
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the project, otherwise false
#
avra_replBelongsTo : path -> boolean
avra_replBelongsTo(*Entity) = _avra_isForAvra(*Entity)


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
avra_replIngestResc : string * boolean
avra_replIngestResc = (avra_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
avra_replReplResc : string * boolean
avra_replReplResc = avra_replIngestResc


# Restrict the Avra resource to files in the Avra collection
pep_resource_resolve_hierarchy_pre(*OUT) {
  on (avra_RESC != ipc_DEFAULT_RESC
      && $KVPairs.resc_hier == avra_RESC
      && !_avra_isForAvra($KVPairs.logical_path)) {
    *msg = 'CYVERSE ERROR:  ' ++ avra_RESC ++ ' usage is limited to the Avra collection, '
           ++ str(avra_BASE_COLL);

    failmsg(-32000, *msg);
  }
}
