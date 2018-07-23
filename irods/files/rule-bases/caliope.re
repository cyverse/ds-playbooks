# VERSION 1
#
# These are the custom rules for the Caliope project

@include "caliope-env"


_caliope_logMsg(*Msg) {
  writeLine('serverLog', 'CALIOPE: *Msg');
}


_caliope_ingest(*Uploader, *TarPath) {
  _caliope_logMsg('ingesting *TarPath for *Uploader');

  *zoneArg = execCmdArg(ipc_ZONE);
  *uploaderArg = execCmdArg(*Uploader);
  *tarArg = execCmdArg(*TarPath);
  *args= "*zoneArg *uploaderArg *tarArg";
  *status = errorcode(msiExecCmd("caliope-ingest", *args, "null", "null", "null", *out));

  msiGetStderrInExecCmdOut(*out, *resp);

  foreach (*err in split(*resp, '\n')) {
    _caliope_logMsg(*err);
  }

  if (*status != 0) {
    failmsg(*status, 'CALIOPE: failed to fully ingest *TarPath');
  }
}


caliope_acPostProcForPut {
  if (str(caliope_BASE_COLL) != '') {
    if ($objPath like regex '^' ++ str(caliope_BASE_COLL) ++ '/[^/]*/Uploads/[^/]*\\.tar$') {
      _caliope_logMsg('scheduling ingest of $objPath for $userNameClient');

      delay("<PLUSET>1s</PLUSET><EF>1s REPEAT 0 TIMES</EF>") {
        _caliope_ingest($userNameClient, $objPath);
      }
    }
  }
}
