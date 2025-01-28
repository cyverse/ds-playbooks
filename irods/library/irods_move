#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# © 2024 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

"""This is the ansible module irods_move."""

import ssl
from ansible.module_utils.basic import AnsibleModule
from irods.session import iRODSSession


DOCUMENTATION = r'''
---
module: irods_move

short_description: Rename data collection/object

version_added: "2.11.11"

description: An ansible module for renaming a data object or collection. To
ensure idempotency, if the source doesn't exist, but the destination does, it
is assumed that the move already happened and the module succeeds.

options:
    source:
        description: the current absolute path to the collection or data object
        type: str
    destination:
        description: the new absolute path to the collection or data object
        type: str

extends_documentation_fragment:
    - my_namespace.my_collection.my_doc_fragment_name

author:
    - Fenn Garnett (@Fennersteel)
'''

EXAMPLES = '''
# Move iRODS data object/collection
- name: move data object/collection
    irods_move:
        source: /testing/home/rods/test_src
        destination: /testing/home/rods/test_dst
        host: "{{ groups['irods_catalog'][0] }}"
        port: 1247
        admin_user: rods
        admin_password: rods
        zone: testing
'''

RETURN = '''
exc:
    description:
        - type of last iRODS exception thrown
        - empty string when none is thrown
    type: str
    returned: always
exc_msg:
    description:
        - message of the last iRODS exception thrown
        - empty string when none is thrown
    type: str
    returned: always
'''


class IRODSMoveModule:
    """
    Module class
    """

    def __init__(self):
        """
        Initialize module
        """

        self.module_args = {
            'source': {'type': 'str', 'required': True},
            'destination': {'type': 'str', 'required': True},
            'host': {'type': 'str', 'required': True},
            'port': {'type': 'int', 'required': True},
            'admin_user': {'type': 'str', 'no_log': True, 'required': True},
            'admin_password': {'type': 'str', 'no_log': True, 'required': True},
            'zone': {'type': 'str', 'required': True},
        }

        self.result = {'changed': False, 'exc': '', 'exc_msg': ""}

        self.module = AnsibleModule(
            argument_spec=self.module_args,
            supports_check_mode=True
        )

        self.ssl_context = ssl.create_default_context(
            purpose=ssl.Purpose.SERVER_AUTH, cafile=None, capath=None,
            cadata=None)

        self.ssl_settings = {"ssl_context": self.ssl_context}

        self.session = None

    def run_module(self):
        """This performs the business logic of the module."""
        if self.module.check_mode:
            self.module.exit_json(**self.result)

        try:
            self._init_session()
            self._rename()
            self.module.exit_json(**self.result)
        except Exception as exc:  # pylint: disable=broad-exception-caught
            self.result["exc"] = type(exc).__name__
            self.result["exc_msg"] = str(exc)
            self._fail(
                f"Unable to move irods object/collection {self.module.params['source']}", exc)

    def _rename(self):
        # rename collection
        path = self.module.params["source"]
        new_path = self.module.params["destination"]
        path_exists = self.session.collections.exists(path)
        new_path_exists = self.session.collections.exists(new_path)

        if path_exists or not new_path_exists:
            self.session.collections.move(path, new_path)
            self.result["changed"] = True

    def _fail(self, msg, err=None):
        """
        Failure routine, called when the operation failed
        """
        if self.session:
            self.session.cleanup()

        if err:
            self.module.fail_json(msg=msg + "\n" + str(err), **self.result)
        else:
            self.module.fail_json(msg=msg, **self.result)

    def _init_session(self):
        self.session = iRODSSession(
            source=self.module.params["source"],
            destination=self.module.params["destination"],
            host=self.module.params["host"],
            port=self.module.params["port"],
            user=self.module.params["admin_user"],
            password=self.module.params["admin_password"],
            zone=self.module.params["zone"],
            **self.ssl_settings)


def main():
    """
    Entrypoint of the Ansible module
    """
    module = IRODSMoveModule()
    module.run_module()


if __name__ == '__main__':
    main()
