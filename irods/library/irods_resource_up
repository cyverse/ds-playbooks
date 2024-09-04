#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Copyright: © 2024, The Arizona Board of Regents
# Standard BSD License | CyVerse (see https://cyverse.org/license)

"""Provides irods_resource_up Ansible module"""

from abc import ABC, abstractmethod
from typing import Optional

from ansible.module_utils.basic import AnsibleModule

from irods.exception import CAT_INVALID_AUTHENTICATION, ResourceDoesNotExist
from irods.resource import iRODSResource
from irods.session import iRODSSession

DOCUMENTATION = r'''
---
module: irods_resource_up

short_description: Change status of resource and parents to "up"

version added: "2.11.11"

description: >
    An ansible module for changing the status of a resource and
    all of its parents to up.

options:
    resource:
        description:
            - the resource to modify
        required: true
        type: object

requirements:
    - python-irodsclient>=0.8.2

author:
    - Fenn Garnett (@Fennersteel)
'''

EXAMPLE = r'''
---
- name: Set status to "up"
    irods_resource_up:
        resource: ingestRes
        host: cyverse.org
        port: 1247
        admin_user: rods
        admin_password: 1234
        zone: tempZone
'''

RETURN = r'''
---
resource:
    description: the name of the resource provided as an input argument
    type: str
    returned: always
'''


_ARG_SPEC = {
    'resource': {
        'type': 'str',
        'required': True,
    },
    'host': {
        'type': 'str',
        'required': True,
    },
    'port': {
        'type': 'int',
        'required': True,
    },
    'admin_user': {
        'type': 'str',
        'required': True,
        'no_log': True,
    },
    'admin_password': {
        'type': 'str',
        'required': True,
        'no_log': True,
    },
    'zone': {
        'type': 'str',
        'required': True
    },
}


class Request:
    """This class encapsulates the parameters passed into this module. """

    def __init__(self, mod_params: dict):
        self._host = mod_params['host']
        self._port = mod_params['port']
        self._zone = mod_params['zone']
        self._admin_user = mod_params['admin_user']
        self._admin_password = mod_params['admin_password']
        self._resc = mod_params['resource']

    @property
    def host(self) -> str:
        """the iRODS hostname or IP address to connect to"""
        return self._host

    @property
    def port(self) -> int:
        """the TCP port to use when connecting to iRODS"""
        return self._port

    @property
    def zone(self) -> str:
        """the iRODS zone used for authentication"""
        return self._zone

    @property
    def admin_user(self) -> str:
        """the rodsadmin username to authorize the operations"""
        return self._admin_user

    @property
    def admin_password(self) -> str:
        """the password to authenticate the rodsadmin user"""
        return self._admin_password

    @property
    def resc(self) -> str:
        """the resource to bring up"""
        return self._resc


class Irods(ABC):
    """
    This is the interface to an object that provides required iRODS requests.

    This interface has been separated from the _IrodsImpl class so that it can be mocked for
    testing.
    """

    @abstractmethod
    def get_resc(self, name: str) -> Optional[iRODSResource]:
        """
        Return an iRODS resource object for the resource with a given name.

        Args:
            name  the name of the resource to retrieve

        Returns:
            If there is a resource in the local zone with the given name, it return an iRODS
            resource object for it, otherwise it returns None.
        """


class _IrodsImpl(Irods):

    def __init__(self, request):
        self._session = iRODSSession(
            host=request.host,
            port=request.port,
            zone=request.zone,
            user=request.admin_user,
            password=request.admin_password)

    def __enter__(self):
        return self

    def __exit__(self, _exn_type, _exn_value, _traceback):
        self._session.cleanup()

    def get_resc(self, name):
        return self._session.resources.get(name)


class _ResourceUnknown(Exception):

    def __init__(self, resc):
        self._resc = resc

    def __str__(self):
        return f"Resource {self._resc} does not exist"


def _ensure_resource_up(resc):
    if resc.status == "up":
        return False

    resc.modify('status', "up")
    return True


def _resource_up_recurse(irods, resc):
    changed = _ensure_resource_up(resc)

    if resc.parent_name:
        changed = _resource_up_recurse(irods, irods.get_resc(resc.parent_name)) or changed

    return changed


def _resource_up(irods, resc_name):
    resc = irods.get_resc(resc_name)

    if not resc:
        raise _ResourceUnknown(resc_name)

    return _resource_up_recurse(irods, resc)


def main() -> None:
    """Entrypoint of Ansible module"""
    ansible = AnsibleModule(argument_spec=_ARG_SPEC, supports_check_mode=True)
    request = Request(ansible.params)
    result = {'resource': request.resc, 'changed': False}

    if ansible.check_mode:
        ansible.exit_json(**result)
    else:
        with _IrodsImpl(request) as irods:
            try:
                result['changed'] = _resource_up(irods, request.resc)
                ansible.exit_json(**result)
            except CAT_INVALID_AUTHENTICATION:
                ansible.fail_json(msg="The provided admin credentials are invalid.", **result)
            except ResourceDoesNotExist:
                ansible.fail_json(msg=f"The resource {request.resc} doesn't exist.", **result)


if __name__ == '__main__':
    main()
