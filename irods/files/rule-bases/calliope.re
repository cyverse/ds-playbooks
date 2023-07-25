# VERSION 3
#
# These are the custom rules for the Calliope project


_calliope_TAR_REGEX =
  '^/' ++ cyverse_ZONE ++ '/home/[^/]+/calliope_data/collections/[^/]+/uploads/[^/]*\\.tar$'


_calliope_isForCalliope(*Path) = *Path like regex _calliope_TAR_REGEX


_calliope_logMsg(*Msg) {
  writeLine('serverLog', 'CALLIOPE: *Msg');
}


_calliope_ingest(*Uploader, *TarPath) {
  _calliope_logMsg('ingesting *TarPath for *Uploader');

  *zoneArg = execCmdArg(cyverse_ZONE);
  *uploaderArg = execCmdArg(*Uploader);
  *tarArg = execCmdArg(*TarPath);
  *args= "*zoneArg *uploaderArg *tarArg";
  *status = errorcode(msiExecCmd("calliope-ingest", *args, "null", "null", "null", *out));

  msiGetStderrInExecCmdOut(*out, *resp);

  foreach (*err in split(*resp, '\n')) {
    _calliope_logMsg(*err);
  }

  if (*status != 0) {
    failmsg(*status, 'CALLIOPE: failed to fully ingest *TarPath');
  }
}


calliope_dataObjCreated(*User, *_, *DATA_OBJ_INFO) {
  if (_calliope_isForCalliope(*DATA_OBJ_INFO.logical_path)) {
    _calliope_logMsg(
      'scheduling ingest of ' ++ *DATA_OBJ_INP.logical_path ++ ' for ' ++ *User);

# XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See
#       https://github.com/irods/irods/issues/5413.
    #delay("<PLUSET>0s</PLUSET><EF>1s REPEAT 0 TIMES</EF>")
    delay(
      ' <INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
        <PLUSET>0s</PLUSET>
        <EF>1s REPEAT 0 TIMES</EF> ' )
    {_calliope_ingest(*User, *DATA_OBJ_INFO.logical_path);}
  }
}
