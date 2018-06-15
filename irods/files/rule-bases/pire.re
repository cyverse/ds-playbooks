# PIRE project policy
# include this file from within ipc-custom.re

@include 'pire-env'


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
pire_replBelongsTo(*Entity) = str(*Entity) like str(pire_BASE_COLL) ++ '/*'


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
