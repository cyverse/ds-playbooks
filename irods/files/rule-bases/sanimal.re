# VERSION: 1
#
# These are the custom rules for the sanimal project

@include "sanimal-env"


_sanimal_logMsg(*Msg) {
  writeLine('serverLog', 'SANIMAL: *Msg');
}


_sanimal_ingest(*Uploader, *TarPath) {
  _sanimal_logMsg('ingesting *TarPath for *Uploader');

  *zoneArg = execCmdArg(ipc_ZONE);
  *uploaderArg = execCmdArg(*Uploader);
  *tarArg = execCmdArg(*TarPath);
  *args= "*zoneArg *uploaderArg *tarArg";
  *status = errorcode(msiExecCmd("sanimal-ingest", *args, "null", "null", "null", *out));

  msiGetStderrInExecCmdOut(*out, *resp);

  foreach (*err in split(*resp, '\n')) {
    _sanimal_logMsg(*err);
  }

  if (*status != 0) {
    failmsg(*status, 'SANIMAL: failed to fully ingest *TarPath');
  }
}


sanimal_acPostProcForPut {
  if (str(sanimal_BASE_COLL) != '') {
    if ($objPath like regex '^' ++ str(sanimal_BASE_COLL) ++ '/[^/]*/Uploads/[^/]*\\.tar$') {
      _sanimal_logMsg('scheduling ingest of $objPath for $userNameClient');

      delay("<PLUSET>1s</PLUSET><EF>1s REPEAT 0 TIMES</EF>") {
        _sanimal_ingest($userNameClient, $objPath);
      }
    }
  }
}
