# Files Deployed for BisQue Support

This folder contains files deployed for BisQue support. Not all files required for BisQue integration are in this directory. Some haven't been migrated from [irods/files/cmd-common/](../cmd-common) and [irods/files/rule-bases/](../rule-bases).

## Command Scripts

The BisQue rule logic requires two command scripts. One of them can be found externally at [bisque-paths](https://gitlab.cyverse.org/ds/bisque_support/raw/master/bisque_paths/bisque_paths.py). The other, [url-encode-path-segments](var/lib/irods/msiExecCmd_bin/url-encode-path-segments) is used to encode an iRODS path as a URL path.

<!-- TODO: write section on Rule Bases -->