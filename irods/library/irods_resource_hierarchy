#!/usr/bin/env python
# -*- coding: utf-8 -*-

# © 2024 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

"""
This is an ansible module for managing the coordinating resources portion of an
iRODS resource hierarchy.
"""

from abc import ABC, abstractmethod
import ssl
from ssl import Purpose, SSLContext
from typing import Optional

from ansible.module_utils.basic import AnsibleModule
from irods.exception import ResourceDoesNotExist
from irods.resource import iRODSResource
from irods.session import iRODSSession

DOCUMENTATION = r'''
---
module: irods_resource_hierarchy

short_description: This module modifies an iRODS resource hierarchy.

description: >
  This module is able to create a resource hierarchy. It will create any
  coordinating resources that do not exist.

version_added: "2.16.7"

author: Tony Edgin

options:
  admin_password:
    description: a password used to authenticate the rodsadmin user
    required: true
    type: str

  admin_username:
    description: |
      a rodsadmin user to authorize the resource hierarchy modification.
    required: true
    type: str

  hierarchy:
    description: |
      a root resource hierarchy to create or update. It should be a `hierarchy`.
      A `hierarchy` is a dictionary with the following fields.

      `name`:     is the name of the resource being created
      `type`:     the type of resource being created. This is only required if
                  the resource is a coordinating resource that needs to be
                  created or have children added.
      `children`: an optional list of child hierarchy dictionaries, each
                  representing a child resource to be attached
      `context`:  an optional string providing context parameters for the
                  resource being created
    required: true
    type: dict

  host:
    description: This is the FQDN or IP address of the iRODS server to connect to.
    default: localhost.localdomain
    type: str

  port:
    description: This is the TCP port to connect to.
    default: 1247
    type: int

  zone:
    description: the iRODS zone containing the resource hierarchy
    required: true
    type: str

requirements:
  -  python-irodsclient
'''

RETURN = r'''
'''

EXAMPLES = r'''
# Create an iRODS coordinating resource
- name: Create random root resource
  irods_resource_hierarchy:
    zone: tempZone
    admin_username: rods
    admin_password: rods
    hierarchy:
      name: rootRes
      type: random

# Create a read-only passthru resource to an existing storage resource named
# storageRes.
- name: Create r/o storage access
  irods_resource_hierarchy:
    host: irods.localdowmain
    port: 1250
    zone: tempZone
    admin_username: rods
    admin_password: rods
    hierarchy:
      name: readOnlyRes
      type: passthru
      children:
        - name: storageRes
      context: 'read=1.0;write=0.0'

# Create a random resource with two child resources. Once child, existRes,
# already exists, and the other, newRes, will be created. newRes will be a
# passthru to an existing storage resource, storeRes.
- name: Extend resource hierarchy
  irods_resource_hierarchy:
    host: irods.localdowmain
    zone: tempZone
    admin_username: rods
    admin_password: rods
    hierarchy:
      name: extendedRes
      type: random
      children:
        - name: existRes
        - name: newRes
          type: passthru
          children:
            - name: storeRes

# Add child resource to root resource
- name: Add storage resource
  irods_resource_hierarchy:
    zone: tempZone
    admin_username: rods
    admin_password: rods
    hierarchy:
      name: rootRes
      type: random
      children:
        - name: storeRes
'''


_ARG_SPEC = {
    "admin_password": {
        "type": "str",
        "required": True,
        "no_log": True,
    },
    "admin_username": {
        "type": "str",
        "required": True,
    },
    "hierarchy": {
        "type": "dict",
        "options": {
            "name": {
                "type": "str",
                "required": True,
            },
            "type": {"type": "str"},
            "children": {
                "type": "list",
                "elements": "dict",
            },
            "context": {"type": "str"},
        },
        "required_by": {
            "children": "type",
            "context": "type"
        },
        "required": True,
    },
    "host": {
        "type": "str",
        "default": "localhost.localdomain",
    },
    "port": {
        "type": "int",
        "default": 1247,
    },
    "zone": {
        "type": "str",
        "required": True,
    },
}


class IrodsConnInfo:
    """This class encapsulates the iRODS connection parameters."""

    def __init__(
        self, host: str, port: int, zone: str, username: str, password: str
    ):  # pylint: disable=too-many-arguments
        self._host = host
        self._port = port
        self._zone = zone
        self._username = username
        self._password = password
        self._ssl_context = ssl.create_default_context(Purpose.SERVER_AUTH)

    @property
    def zone(self) -> str:
        """
        This is the iRODS zone that will be used. It is also the zone the user belongs to.
        """
        return self._zone

    @property
    def username(self) -> str:
        """This is the rodsadmin account that authorizes the user changes."""
        return self._username

    @property
    def password(self) -> str:
        """
        This is the password that authenticates the rodsadmin account making the changes.
        """
        return self._password

    @property
    def host(self) -> str:
        """
        This is the domain name or IP address of the iRODS server to connect to.
        """
        return self._host

    @property
    def port(self) -> int:
        """
        This is the TCP port iRODS uses on the server that will be connected to.
        """
        return self._port

    @property
    def ssl_context(self) -> SSLContext:
        """This is the context for TLS connection."""
        return self._ssl_context


class Resource:
    """
    This class encapsulates a resource. If a type is provided, the resource will
    be defined, otherwise it is assumed to be a reference to an existing
    resource.
    """

    def __init__(self, request_param: dict):
        resc_type = request_param.get("type", None)
        context = request_param.get("context", None)
        child_defs = request_param.get("children", None)
        if context is not None or child_defs is not None:
            if not resc_type:
                raise ValueError("a resource type is required if context or children are provided")
        self._name = request_param["name"]
        self._type = resc_type
        self._context = context
        if child_defs is None:
            self._children = None
        else:
            self._children = []
            for child_def in child_defs:
                self._children.append(Resource(child_def))

    @property
    def children(self) -> Optional[list]:
        """these are the children of the root resource"""
        return self._children

    @property
    def context(self) -> Optional[str]:
        """this is the root resource's context value"""
        return self._context

    @property
    def name(self) -> str:
        """the name of the root resource."""
        return self._name

    @property
    def resc_type(self) -> Optional[str]:
        """this is the root's type of resource."""
        return self._type

    def children_provided(self) -> bool:
        """
        Indicates if the resource definitions provides information about child
        resources.
        """
        return self.children is not None

    def has_definition(self) -> bool:
        """
        Indicates whether or not a definition of the resource has been
        provided.
        """
        return self.resc_type is not None


class Request:
    """This class encapsulates the parameters passed into this module. """

    def __init__(self, module_params: dict):
        self._conn_info = IrodsConnInfo(
            host=module_params["host"],
            port=module_params["port"],
            zone=module_params["zone"],
            username=module_params["admin_username"],
            password=module_params["admin_password"])
        self._hierarchy = Resource(module_params['hierarchy'])

    @property
    def irods_conn_info(self) -> IrodsConnInfo:
        """This is the iRODS connection information provided in the request."""
        return self._conn_info

    @property
    def hierarchy(self) -> Resource:
        """
        This is the requested state of the resource hierarchy being modified.
        """
        return self._hierarchy


class Irods(ABC):
    """
    This is the interface to an object that provides required iRODS requests.

    This interface has been separated from the _IrodsImpl class so that it can be mocked for
    testing.
    """

    @abstractmethod
    def add_child_to_resc(self, parent_name: str, child_name: str) -> None:
        """
        Attach a child resource to a parent resource

        Args:
            parent_name  the name of the parent resource
            child_name   the name of the child resource
        """

    @abstractmethod
    def create_resc(self, name: str, resc_type: str, context: str) -> None:
        """
        Creates a coordinating resource.

        Args:
            name       the name of the resource to create
            resc_type  the type of coordinating resource to create
            context    the context to assign to the new resource
        """

    @abstractmethod
    def get_resc(self, name: str) -> iRODSResource:
        """
        Retrieves a resource

        Args:
            name  the name of the resource to retrieve

        Returns:
            It returns the resource.

        Raises:
            ResourceDoesNotExist  when there is no resource with that name in iRODS
        """

    @abstractmethod
    def has_resc(self, name: str) -> bool:
        """
        Determines if a resource exists.

        Args:
            name  the name of the resource

        Returns:
            It returns True of the resource exists, otherwise it returns False.
        """


class _IrodsImpl(Irods):

    def __init__(self, request):
        self._session = iRODSSession(
            host=request.host,
            port=request.port,
            zone=request.zone,
            user=request.username,
            password=request.password,
            ssl_context=ssl.create_default_context(Purpose.SERVER_AUTH))

    def __enter__(self):
        return self

    def __exit__(self, _type, _value, _traceback):
        self._session.cleanup()

    def add_child_to_resc(self, parent_name, child_name):
        self._session.resources.add_child(parent_name, child_name)

    def create_resc(self, name, resc_type, context):
        self._session.resources.create(name, resc_type, context=context)
        self._session.resources.modify(name, 'status', 'down')

    def get_resc(self, name):
        return self._session.resources.get(name)

    def has_resc(self, name):
        try:
            self.get_resc(name)
            return True
        except ResourceDoesNotExist:
            return False


class ResourceHierarchyException(Exception):
    """Indicates that the module failed to create a resource hierarchy"""

    def __init__(self, msg: str):
        super().__init__(msg)


class RescHierMgr:
    """
    This performs the business logic of the module.

    Args:
        irods       the object managing the interaction with iRODS
        resc_state  the requested final state of the hierarchy's root resource

    Returns:
        It returns True, if it made any changes to iRODS, otherwise it returns False.
    """

    def __init__(self, irods: Irods, resc_state: Resource):
        self._irods = irods
        self._resc_state = resc_state
        self._made_change = False

    def made_change(self) -> bool:
        """
        It returns True, if it made any changes to iRODS, otherwise it returns
        False.
        """
        return self._made_change

    def update_irods(self) -> None:
        """Updates iRODS if needed."""
        self._create_hier(self._resc_state)

    def _add_child(self, parent_resc_state: Resource, child_resc_state: Resource) -> None:
        if not self._irods.has_resc(child_resc_state.name):
            raise ResourceHierarchyException(
                f"referenced resource {child_resc_state.name} does not exist")
        child = self._irods.get_resc(child_resc_state.name)
        if child.parent_name:
            if child.parent_name != parent_resc_state.name:
                raise ResourceHierarchyException(
                    f"resource {child_resc_state.name} has existing parent {child.parent_name} !="
                    f" {parent_resc_state.name}")
        else:
            self._irods.add_child_to_resc(parent_resc_state.name, child_resc_state.name)
            self._made_change = True

    def _create_coord_resc(self, resc_state: Resource) -> None:
        if self._irods.has_resc(resc_state.name):
            resc = self._irods.get_resc(resc_state.name)
            if resc_state.resc_type != resc.type:
                raise ResourceHierarchyException(
                    f"resource {resc_state.name} exists with type {resc.type} !="
                    f" {resc_state.resc_type}")
            if resc_state.context != resc.context:
                raise ResourceHierarchyException(
                    f"resource {resc_state.name} exists with context {resc.context} !="
                    f"{resc_state.context}")
        else:
            self._irods.create_resc(resc_state.name, resc_state.resc_type, resc_state.context)
            self._made_change = True

    def _create_hier(self, root_resc_state: Resource) -> None:
        if root_resc_state.has_definition():
            self._create_coord_resc(root_resc_state)
            if self._resc_state.children_provided():
                for child in self._resc_state.children:
                    self._create_hier(child)
                    self._add_child(root_resc_state, child)


def main() -> None:
    """The entrypoint."""
    ansible = AnsibleModule(argument_spec=_ARG_SPEC)
    result = {'changed': False}
    try:
        request = Request(ansible.params)
        with _IrodsImpl(request.irods_conn_info) as irods:
            mgr = RescHierMgr(irods, request.hierarchy)
            mgr.update_irods()
            result['changed'] = mgr.made_change()
        ansible.exit_json(**result)
    except ResourceHierarchyException as e:
        ansible.fail_json(msg=str(e))


if __name__ == "__main__":
    main()
