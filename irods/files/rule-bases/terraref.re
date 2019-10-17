# TerraREF project policy
# include this file from within ipc-custom.re

@include 'terraref-env'

_terraref_isForTerraRef(*Path) =
  let *strName = str(terraref_BASE_COLL) in
  *strName != '' && str(*Path) like *strName ++ '/*'


# Determines if the provided collection or data object belongs to the TerraREF
# project
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the project, otherwise false
#
terraref_replBelongsTo : path -> boolean
terraref_replBelongsTo(*Entity) = _terraref_isForTerraRef(*Entity)


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
terraref_replIngestResc : string * boolean
terraref_replIngestResc = (terraref_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
terraref_replReplResc : string * boolean
terraref_replReplResc = terraref_replIngestResc


# Restrict the TerraREF resource to files in the TerraREF collection
pep_resource_resolve_hierarchy_pre(*OUT) {
  on (terraref_RESC != ipc_DEFAULT_RESC
      && $KVPairs.resc_hier == terraref_RESC
      && !_terraref_isForTerraRef($KVPairs.logical_path)) {
    *msg = 'CYVERSE ERROR:  ' ++ terraref_RESC ++ ' usage is limited to the TerraREF collection, '
           ++ str(terraref_BASE_COLL);

    failmsg(-32000, *msg);
  }
}
