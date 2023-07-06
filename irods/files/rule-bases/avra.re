# Avra project policy
# include this file from within ipc-custom.re

@include 'avra-env'

_avra_isForAvra(*Path) =
  let *answer = false in
  let *avraRes = avra_RESC in
  let *_ = foreach( *rec in
      SELECT META_RESC_ATTR_VALUE
      WHERE RESC_NAME = *avraRes AND META_RESC_ATTR_NAME = 'ipc::hosted-collection'
    ) { *answer = *answer || (*Path like *rec.META_RESC_ATTR_VALUE ++ '/*'); } in
  *answer

# Restrict the Avra resource to files in the Avra collection
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT, *OUT, *OPERATION, *HOST, *PARSER, *VOTE) {
  on (
    avra_RESC != cyverse_DEFAULT_RESC
    && *CONTEXT.resc_hier == avra_RESC
    && ! _avra_isForAvra(*CONTEXT.logical_path)
  ) {
    *msg = 'CYVERSE ERROR: ' ++ *CONTEXT.logical_path ++ ' not allowed on ' ++ avra_RESC ++ '.';
# XXX - Because of https://github.com/irods/irods/issues/6463, an error
# happening in an `ON` condition needs to be captured and sent in the catch-all.
#     cut;
#     failmsg(-32000, *msg);
    temporaryStorage.resource_resolve_hierarchy_err = *msg;
    fail;
# XXX - ^^^
  }
}
