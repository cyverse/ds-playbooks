# DE project policy
# include this file from with ipc-custom.re


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
de_replBelongsTo(*Entity) = str(*Entity) like '/iplant/jobs/*'


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
