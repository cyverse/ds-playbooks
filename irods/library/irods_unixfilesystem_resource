#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Provide irods_unixfilesystem_resource Ansible module"""

import os
from ansible.module_utils.basic import AnsibleModule
from irods.exception import ResourceDoesNotExist
from irods.session import iRODSSession


DOCUMENTATION = r"""
---
module: irods_unixfilesystem_resource

short_description: Create an iRODS Unix filesystem storage resource

version added: "2.11.11"

description: >
  An ansible module for creating an iRODS Unix filesystem storage resource. The
  status will be set only and free space will be initialized only if the
  resource is created.

options:
  name:
    description: the name of the resource
    required: true
    type: str
  host:
    description: >
      the identity of the resource server that will host this resource
    required: true
    type: str
  vault:
    description: the absolute path to the root directory of the vault
    required: true
    type: path
  context:
    description: any context to attach to this resource
    required: false
    type: str
  status:
    description: starting status 'up' or 'down'
    required: false
    default: "up"
    type: str
  init_free_space:
    description: whether or not to initialize freespace
    required: false
    default: false
    type: bool

requirements:
  - python-irodsclient

author: Fenn Garnett (@Fennersteel)
"""


class _IRODSError(Exception):

    def __init__(self, message, cause=None):
        super().__init__()
        self._message = message
        self._cause = cause

    def __str__(self):
        return self._message

    @property
    def cause(self):
        """the exception causing of the error"""
        return self._cause


class IRODSUnixResourceModule:  # pylint: disable=too-few-public-methods
    """
    Module class
    """

    def __init__(self):
        """
        Initialize the module
        """
        module_args = dict(
            name=dict(type="str", required=True),
            host=dict(type="str", required=True),
            vault=dict(type="path", required=True),
            context=dict(type="str", required=False, default=""),
            status=dict(type="str", required=False, default="up"),
            init_free_space=dict(type="bool", required=False, default=False),
        )
        self._module = AnsibleModule(
            argument_spec=module_args, supports_check_mode=True)
        self._result = dict(changed=False, response="", exc="", exc_msg="")

    def run_module(self):
        """
        Main module operative function
        """
        if self._module.check_mode:
            self._module.exit_json(**self._result)
            return
        try:
            with self._init_session() as session:
                self._ensure_resource_created(session)
            self._module.exit_json(**self._result)
        except _IRODSError as err:
            if err.cause:
                self._result["exc"] = type(err.cause).__name__
                self._result["exc_msg"] = str(err.cause)
            fail_msg = "\n".join(
                filter(lambda x: x != "", [str(err), self._result["exc_msg"]]))
            self._module.fail_json(msg=fail_msg, **self._result)

    def _init_session(self):
        try:
            return iRODSSession(
                irods_env_file='/var/lib/irods/.irods/irods_environment.json')
        except Exception as exc:  # pylint: disable=broad-except
            raise _IRODSError(   # pylint: disable=raise-missing-from
                message="unable to connect to iRODS server", cause=exc)

    def _ensure_resource_created(self, session):
        if self._resource_exists(session):
            self._verify_resource_same(session)
        else:
            self._create_resource(session)

    def _create_resource(self, session):
        try:
            session.resources.create(
                name=self._module.params["name"],
                resource_type="unixfilesystem",
                host=self._module.params["host"],
                path=self._module.params["vault"],
                context=self._module.params["context"],
            )
            session.resources.modify(
                name=self._module.params["name"],
                attribute="status",
                value=self._module.params["status"],
            )
            if self._module.params["init_free_space"]:
                session.resources.modify(
                    name=self._module.params["name"],
                    attribute="freespace",
                    value=self._get_free_space(),
                )
            self._result["changed"] = True
        except Exception as exc:  # pylint: disable=broad-except
            msg = "unable to create resource"
            try:
                if self._resource_exists(session):
                    session.resources.remove(self._module.params["name"])
            except Exception:  # pylint: disable=broad-except
                msg = "unable to fully create resource"
            raise _IRODSError(message=msg, cause=exc)  # pylint: disable=raise-missing-from # noqa

    def _resource_exists(self, session):
        try:
            session.resources.get(self._module.params["name"])
            return True
        except ResourceDoesNotExist:
            return False

    def _verify_resource_same(self, session):
        resc = session.resources.get(self._module.params["name"])
        if resc.type != "unixfilesystem" and resc.type != "unix file system":
            raise _IRODSError("Resource already exists with different type")
        if resc.location != self._module.params["host"]:
            raise _IRODSError("Resource already exists on different host")
        if resc.vault_path != self._module.params["vault"]:
            raise _IRODSError("Resource already exists in different vault")
        if (resc.context or "") != self._module.params["context"]:
            raise _IRODSError("Resource already exists with different context")

    def _get_free_space(self):
        statvfs = os.statvfs(self._module.params["vault"])
        return statvfs.f_frsize * statvfs.f_bfree


def main():
    """
    Entrypoint of the Ansible module
    """
    IRODSUnixResourceModule().run_module()


if __name__ == "__main__":
    main()
