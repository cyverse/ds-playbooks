# These are the custom rules for the Sparc'd project

@include "sparcd-env"

_sparcd_MAX_RETRIES = 2

_sparcd_PERM = 'own'

_sparcd_encode_subject(*Subject) =
  let *enc = '' in
  let *len = strlen(*Subject) in
  let *pos = 0 in
  let *_ = while (*len > *pos) {
    *c = substr(*Subject, *pos, *pos + 1);
    *enc = *enc ++ if *c like regex '[a-zA-Z0-9,./ ]' then *c else '.';
    *pos = *pos + 1 } 
  in *enc

_sparcd_encode_url(*Url) =
  let *enc = '' in
  let *len = strlen(*Url) in
  let *pos = 0 in
  let *_ = while (*len > *pos) {
    *c = substr(*Url, *pos, *pos + 1);
    *enc = *enc ++ if *c == ' ' then '%20' else *c;
    *pos = *pos + 1 }
  in *enc

 
_sparcd_logMsg(*Msg) {
  writeLine('serverLog', 'SPARCD: *Msg');
}


_sparcd_notify(*Subject, *Body) {
  *encSubj = 'SPARCD ' ++ _sparcd_encode_subject(*Subject);
  if (0 != errorcode(msiSendMail(sparcd_REPORT_EMAIL_ADDR, *encSubj, *Body))) {
    writeLine('serverLog', 'SPARCD: failed to send notification');
  }
}


_sparcd_ingest(*Uploader, *TarPath) {
  _sparcd_logMsg('ingesting *TarPath for *Uploader');

  *coll = trimr(*TarPath, '-');
  *url = _sparcd_encode_url('https://' ++ sparcd_WEBDAV_HOST ++ '/dav' ++ *coll ++ '/');   

  *zoneArg = execCmdArg(ipc_ZONE);
  *adminArg = execCmdArg(sparcd_ADMIN);
  *uploaderArg = execCmdArg(*Uploader);
  *tarArg = execCmdArg(*TarPath);
  *args = "*zoneArg *adminArg *uploaderArg *tarArg";

  *status = 0;
  *resp = '';
  for (*try = 0; *try <= _sparcd_MAX_RETRIES; *try = *try + 1) {
    *status = errormsg(msiExecCmd("sparcd-ingest", *args, ipc_RE_HOST, "null", "null", *out), *err);
    
    if (*status == 0) {
      break;
    } else {
      _sparcd_logMsg(*err);
      msiGetStderrInExecCmdOut(*out, *resp);

      foreach (*err in split(*resp, '\n')) {
        _sparcd_logMsg(*err);
      }

      if (*try < _sparcd_MAX_RETRIES) {
        _sparcd_logMsg('retrying ingest of *TarPath for *Uploader');
      }
    }
  }

  if (*status == 0) {
    _sparcd_notify(
      "ingest success for *TarPath", 
      "*Uploader successfully ingested the image bundle *TarPath into *coll (*url)." );

    _sparcd_logMsg('finished ingesting *TarPath for *Uploader');
  } else {
    *notificationBody =
      "*Uploader failed to completely ingest the image bundle *TarPath into *coll (*url). The " ++
      "error is as follows.\n" ++
      "\n" ++ 
      *resp;

    _sparcd_notify("ingest failure for *TarPath", *notificationBody);

    *msg = 'failed to fully ingest *TarPath for *Uploader';
    _sparcd_logMsg(*msg);
    failmsg(*status, 'SPARCD: *msg');
  }
}


_sparcd_isForSparcd(*Path) =
  let *strBase = str(sparcd_BASE_COLL) in *strBase != '' && *Path like *strBase ++ '/*'


_sparcd_handle_new_object(*User, *Object) {
  if (_sparcd_isForSparcd(*Object)) {
    ipc_giveAccessObj(sparcd_ADMIN, _sparcd_PERM, *Object);

    if (*Object like regex '^' ++ str(sparcd_BASE_COLL) ++ '/[^/]*/Uploads/[^/]*\\.tar$') {
      _sparcd_logMsg('scheduling ingest of *Object for *User');

# XXX - The rule engine plugin must be specified. This is fixed in iRODS 4.2.9. See 
#       https://github.com/irods/irods/issues/5413.
#       delay("<PLUSET>1s</PLUSET><EF>1s REPEAT 0 TIMES</EF>")
      delay(
        ' <INST_NAME>irods_rule_engine_plugin-irods_rule_language-instance</INST_NAME>
          <PLUSET>0s</PLUSET>
          <EF>0s REPEAT 0 TIMES</EF> ' 
      ) {_sparcd_ingest(*User, *Object)}
# XXX - ^^^
    }
  }
}


sparcd_acPostProcForCollCreate {
  if (_sparcd_isForSparcd($collName)) {
    ipc_giveAccessColl(sparcd_ADMIN, _sparcd_PERM, $collName);
  }
}


sparcd_dataObjCreated(*User, *_, *DATA_OBJ_INFO) {
  _sparcd_handle_new_object(*User, *DATA_OBJ_INFO.logical_path);
}
