# VERSION 3
#
# These are the custom rules for the Calliope project


_calliope_TAR_REGEX =
  '^/' ++ ipc_ZONE ++ '/home/[^/]+/calliope_data/collections/[^/]+/uploads/[^/]*\\.tar$'


_calliope_isForCalliope(*Path) = *Path like regex _calliope_TAR_REGEX


_calliope_logMsg(*Msg) {
  writeLine('serverLog', 'CALLIOPE: *Msg');
}


_calliope_ingest(*Uploader, *TarPath) {
  _calliope_logMsg('ingesting *TarPath for *Uploader');

  *zoneArg = execCmdArg(ipc_ZONE);
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


calliope_acPostProcForPut {
  if (_calliope_isForCalliope($objPath)) {
    _calliope_logMsg('scheduling ingest of $objPath for $userNameClient');

    delay("<PLUSET>0s</PLUSET><EF>1s REPEAT 0 TIMES</EF>")
    {_calliope_ingest($userNameClient, $objPath);}
  }
}


# Add a call to this rule from inside the acPostProcForFilePathReg PEP.
calliope_acPostProcForFilePathReg {
  if (_calliope_isForCalliope($objPath)) {
    _calliope_logMsg('scheduling ingest of $objPath for $userNameClient');

    delay("<PLUSET>0s</PLUSET><EF>1s REPEAT 0 TIMES</EF>")
    {_calliope_ingest($userNameClient, $objPath);}
  }
}
