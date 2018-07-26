# VERSION 1
#
# These are the custom rules for the Calliope project

@include "calliope-env"


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
  if (str(calliope_BASE_COLL) != '') {
    if ($objPath like regex '^' ++ str(calliope_BASE_COLL) ++ '/[^/]*/Uploads/[^/]*\\.tar$') {
      _calliope_logMsg('scheduling ingest of $objPath for $userNameClient');

      delay("<PLUSET>0s</PLUSET><EF>1s REPEAT 0 TIMES</EF>") {
        _calliope_ingest($userNameClient, $objPath);
      }
    }
  }
}
