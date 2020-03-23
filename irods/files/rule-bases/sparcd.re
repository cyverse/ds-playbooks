# VERSION: 5
#
# These are the custom rules for the Sparc'd project

@include "sparcd-env"


_sparcd_logMsg(*Msg) {
  writeLine('serverLog', 'SPARCD: *Msg');
}


_sparcd_ingest(*Uploader, *TarPath) {
  _sparcd_logMsg('ingesting *TarPath for *Uploader');

  *zoneArg = execCmdArg(ipc_ZONE);
  *adminArg = execCmdArg(sparcd_ADMIN);
  *uploaderArg = execCmdArg(*Uploader);
  *tarArg = execCmdArg(*TarPath);
  *args = "*zoneArg *adminArg *uploaderArg *tarArg";
  *status = errormsg(msiExecCmd("sparcd-ingest", *args, "null", *TarPath, "null", *out), *err);

  if (*status != 0) {
    _sparcd_logMsg(*err);

    msiGetStderrInExecCmdOut(*out, *resp);

    foreach (*err in split(*resp, '\n')) {
      _sparcd_logMsg(*err);
    }

    failmsg(*status, 'SPARCD: failed to fully ingest *TarPath');
  }
}


sparcd_acPostProcForPut {
  if (str(sparcd_BASE_COLL) != '') {
    if ($objPath like regex '^' ++ str(sparcd_BASE_COLL) ++ '/[^/]*/Uploads/[^/]*\\.tar$') {
      ipc_giveAccessObj(sparcd_ADMIN, 'own', $objPath);

      _sparcd_logMsg('scheduling ingest of $objPath for $userNameClient');

      delay("<PLUSET>1s</PLUSET><EF>1s REPEAT 0 TIMES</EF>")
      {_sparcd_ingest($userNameClient, $objPath);}
    }
  }
}


acSetChkFilePathPerm {
  on (str(sparcd_BASE_COLL) != ''
      && $objPath like regex '^' ++ str(sparcd_BASE_COLL) ++ '/[^/]*/Uploads/.*$') {
    msiSetChkFilePathPerm('noChkPathPerm');
  }
}
