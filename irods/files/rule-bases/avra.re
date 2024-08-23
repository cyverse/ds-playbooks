# Avra project policy
#
# © 2024 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

@include 'avra-env'


_avra_isForAvra(*Path) =
	let *answer = false in
	let *avraRes = avra_RESC in
	let *_ = foreach( *rec in
			SELECT META_RESC_ATTR_VALUE
			WHERE RESC_NAME = *avraRes AND META_RESC_ATTR_NAME = 'ipc::hosted-collection'
		) { *answer = *answer || (*Path like *rec.META_RESC_ATTR_VALUE ++ '/*'); } in
	*answer


### DYNAMIC PEPS ###

## RESOURCE ##

# RESOLVE HIERARCHY

# This rule is provides the preprocessing logic for determine which storage
# resource to choose for a replica.
#
# This branch restricts the AVRA resource to files in the AVRA collection.
#
# Parameters:
#  Instance  (string) unused
#  Context   (`KeyValuePair_PI`) the resource plugin context
#  OUT       (`KeyValuePair_PI`) unused
#  Op        (string) unused
#  Host      (string) unused
#  PARSER    (`KeyValuePair_PI`) unused
#  VOTE      (float) unused
#
# XXX - Because of https://github.com/irods/irods/issues/6463
# # Error Codes:
# #  -32000 (SYS_INVALID_RESC_INPUT)  this is returned when an error occurred in
# #                                   one of the on branches of this rule
# temporaryStorage:
#  resource_resolve_hierarchy_err  this is used to store an error message when an attempt is made to
#                                  to store a replica on the AVRA resource that should be.
# XXX - ^^^
#
pep_resource_resolve_hierarchy_pre(*Instance, *Context, *OUT, *Op, *Host, *PARSER, *VOTE) {
	on (
		avra_RESC != cyverse_DEFAULT_RESC
		&& *Context.resc_hier == avra_RESC
		&& ! _avra_isForAvra(*Context.logical_path)
	) {
		*msg = 'CYVERSE ERROR: ' ++ *Context.logical_path ++ ' not allowed on ' ++ avra_RESC ++ '.';
# XXX - Because of https://github.com/irods/irods/issues/6463, an error
# happening in an `ON` condition needs to be captured and sent in the catch-all.
# 		cut;
# 		failmsg(-32000, *msg);
		temporaryStorage.resource_resolve_hierarchy_err = *msg;
		fail;
# XXX - ^^^
	}
}
