#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import print_function

import json
import sys

from irods import upgrade_configuration
from irods.configuration import IrodsConfig

Version = "/var/lib/irods/VERSION.json.dist"
IrodsEnv = "/var/lib/irods/.irods/irods_environment.json"
ServerCfg = "/etc/irods/server_config.json"
HostsCfg = "/etc/irods/hosts_config.json"
HostAccessCtrlCfg = "/etc/irods/host_access_control_config.json"


def main():
    try:
        cfg = IrodsConfig()

        with open(Version, 'r') as f:
            new_version = json.load(f)

        new_version['previous_version'] = {
            'schema_name': "VERSION",
            'schema_version': "v2",
            "irods_version": "4.1.10",
            'commit_id': "bb48cd38c4a543ad8bf45082fa06b15290378b50",
            'catalog_schema_version': 4,
            'configuration_schema_version': 2}

        upgrade_configuration.upgrade_config_file(cfg, ServerCfg, new_version)
        upgrade_configuration.upgrade_config_file(cfg, HostsCfg, new_version)
        upgrade_configuration.upgrade_config_file(cfg, HostAccessCtrlCfg, new_version)
        cfg.clear_cache()

        upgrade_configuration.upgrade_config_file(
            cfg, IrodsEnv, new_version, schema_name="service_account_environment")
    except Exception as e:
        print(e, file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
