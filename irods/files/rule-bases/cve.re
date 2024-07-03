# This rule base contains workarounds CVEs.
#
# © 2024 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.


# Bug in the parsing of input to irodsServerMonPerf perl script
# - https://www.cve.org/CVERecord?id=CVE-2024-38461
# - https://github.com/irods/irods/issues/7652
# - authenticated user can escalate to service account (admin) and execute remote code
#
# This can be removed after upgrading to 4.3.3.
msiServerMonPerf(*_1, *_2) {
	writeLine('serverLog', 'intercepted msiServerMonPerf call');
}


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
