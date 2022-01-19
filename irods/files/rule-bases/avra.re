# Avra project policy
# include this file from within ipc-custom.re

@include 'avra-env'

_avra_isForAvra(*Path) =
  let *strName = str(avra_BASE_COLL) in
  *strName != '' && str(*Path) like *strName ++ '/*'

# Restrict the Avra resource to files in the Avra collection
pep_resource_resolve_hierarchy_pre(*INSTANCE, *CONTEXT, *OUT, *OPERATION, *HOST, *PARSER, *VOTE) {
  on (
    avra_RESC != ipc_DEFAULT_RESC
    && *CONTEXT.resc_hier == avra_RESC
    && ! _avra_isForAvra(*CONTEXT.logical_path)
  ) {
    *msg = 'CYVERSE ERROR:  ' ++ avra_RESC ++ ' usage is limited to the Avra collection, '
      ++ str(avra_BASE_COLL);
    cut;
    failmsg(-32000, *msg);
  }
}
