# aegis project policy
# include this file from within ipc-custom.re

@include 'aegis-env'


# Determines if the provided collection or data object belongs to the aegis
# project
#
# Parameters:
#  Entity  the absolute path to the collection or data object
#
# Return:
#  true if the collection or data object belongs to the project, otherwise false
#
aegis_replBelongsTo : path -> boolean
aegis_replBelongsTo(*Entity) = str(*Entity) like str(aegis_BASE_COLL) ++ '/*'


# Returns the resource where newly ingested files will be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
aegis_replIngestResc : string * boolean
aegis_replIngestResc = (aegis_INGEST_RESC, false)


# Returns the resource where the second and subsequent replicas of a file will
# be stored
#
# Return:
#   a tuple where the first value is the name of the resource and the second is
#   a flag indicating whether or not this resource choice may be overridden by
#   the user.
#
aegis_replReplResc : string * boolean
aegis_replReplResc = (aegis_REPL_RESC, false)
