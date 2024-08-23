#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright: © 2024, The Arizona Board of Regents
# Standard BSD License | CyVerse (see https://cyverse.org/license)

"""an ansible module for initializing the clerver zone connection"""

import os
from os import path
import subprocess
from subprocess import CalledProcessError

from ansible.module_utils.basic import AnsibleModule


DOCUMENTATION = r'''
---
module: irods_clerver_auth

short_description: This module initializes the clerver connection.

description: >
    This module initializes the clerver's zone connection to iRODS.

version_added: "2.16.7"

authors:
    - Tony Edgin

options:
    password:
        description: the clerver user's password
        type: str
        required: true
    provider:
        description: the FQDN of the iRODS server the clerver will use for authentication
        type: str
        required: false
        default: localhost
'''

EXAMPLES = r'''
- name: Authenticate clerver user with iRODS server running locally
  irods_clerver_auth:
    password: rods

- name: Authenticate clerver user with iRODS server running remotely
  irods_clerver_user:
    host: test.cyverse.org
    password: rods
'''

RETURN = r'''
'''


_ARG_SPEC = {
    "host": {
        "type": "str",
        "required": False,
        "default": "localhost",
    },
    "password": {
        "type": "str",
        "required": True,
        "no_log": True,
    },
}


def _call_iinit(arg: str = None, input_text: str = None) -> str:
    args = ["iinit"]

    if arg:
        args.append(arg)

    cmd = subprocess.run(
        args=args,
        encoding='utf-8',
        input=input_text,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=True)
    return cmd.stdout


def _get_auth_file() -> str:
    try:
        for line in _call_iinit(arg="-l").splitlines():
            if line.startswith("irods_authentication_file "):
                return line.removeprefix("irods_authentication_file - ")

        return f'{os.environ["HOME"]}/.irods/.irodsA'
    except CalledProcessError as cpe:
        msg = f"Failed to determine if the authentication file exists: {cpe.stderr}"
        raise RuntimeError(msg) from cpe


def _authenticate_clever(host: str, password: str) -> bool:
    if path.isfile(_get_auth_file()):
        if subprocess.run("ils", check=False).returncode == 0:
            return False

    try:
        if host != "localhost":
            os.environ["IRODS_HOST"] = host

        _call_iinit(input_text=password)
        return True
    except CalledProcessError as cpe:
        raise RuntimeError(f"Failed to set new password: {cpe.stderr}") from cpe


def main() -> None:
    """Entrypoint of the ansible module"""
    ansible = AnsibleModule(argument_spec=_ARG_SPEC)
    result = {'changed': False}

    try:
        result['changed'] = _authenticate_clever(ansible.params['host'], ansible.params['password'])
        ansible.exit_json(**result)
    except RuntimeError as e:
        ansible.fail_json(msg=str(e))


if __name__ == '__main__':
    main()
