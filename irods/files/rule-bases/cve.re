# This rule base contains workarounds CVEs.
#
# © 2025 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.


# Bug in the parsing of input to msiSendMail
# - https://www.cve.org/CVERecord?id=CVE-2024-38462
# - https://github.com/irods/irods/issues/7651
# - https://github.com/irods/irods/issues/7562
# - authenticated user can escalate to service account (admin) and execute remote code
#
# This can be removed after upgrading to 4.3.2
msiSendMail(*_1, *_2, *_3) {
	writeLine('serverLog', 'intercepted msiSendMail call');
}


# Bug in the parsing of input to irodsServerMonPerf perl script
# - https://www.cve.org/CVERecord?id=CVE-2024-38461
# - https://github.com/irods/irods/issues/7652
# - authenticated user can escalate to service account (admin) and execute remote code
#
# This can be removed after upgrading to 4.3.3.
msiServerMonPerf(*_1, *_2) {
	writeLine('serverLog', 'intercepted msiServerMonPerf call');
}


# There is a security hole in `icp -p` that allows an overwrite to escape the
# iRODS access control and overwrite a file not accessible to the user. This
# prevents `icp -p` from being able to write to client-submitted physical paths.
#
# This can be removed after upgrading to 4.3.5
#
# Parameters:
#  Instance        (string) unused
#  Comm            (`KeyValuePair_PI`) unused
#  DataObjCopyInp  (`KeyValuePair_PI`) information related to copy operation
#  TransStat       (unknown) unused
#
# Error Codes:
#  -31000 (SYS_INVALID_FILE_PATH)
#
pep_api_data_obj_copy_pre(*Instance, *Comm, *DataObjCopyInp, *TransStat) {
	on (errorcode(*DataObjCopyInp.dst_filePath) == 0) {
		cut;
		failmsg(-31000, 'CYVERSE ERROR: no physical path allowed');
	}
}


# There is a security hole in `iput -p` that allows an overwrite to escape the
# iRODS access control and overwrite a file not accessible to the user. This
# prevents `iput -p` from being able to write to client-submitted physical paths.
#
# This can be removed after upgrading to 4.3.5
#
# Parameters:
#  Instance        (string) unused
#  Comm            (`KeyValuePair_PI`) unused
#  DataObjInp      (`KeyValuePair_PI`) information related to the data object
#  DataObjInpBBuf  (unknown) unused
#  PORTAL_OPR_OUT  (unknown) unused
#
# Error Codes:
#  -31000 (SYS_INVALID_FILE_PATH)
#
pep_api_data_obj_put_pre(*Instance, *Comm, *DataObjInp, *DataObjInpBBuf, *PORTAL_OPR_OUT) {
	on (errorcode(*DataObjInp.filePath) == 0) {
		cut;
		failmsg(-31000, 'CYVERSE ERROR: no physical path allowed');
	}
}
