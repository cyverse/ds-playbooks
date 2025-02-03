#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# © 2025 The Arizona Board of Regents on behalf of The University of Arizona.
# For license information, see https://cyverse.org/license.

"""This is a library to support testing iRODS rule logic."""

import enum
from enum import Enum
from os import environ
import pprint
from typing import List, Optional, Tuple
from unittest import TestCase

from paramiko import AutoAddPolicy, SSHClient

from irods.message import RErrorStack
from irods.rule import Rule
from irods.session import iRODSSession


#
# SSH
#

_ssh: SSHClient


def open_ssh(remote_host):
    """
    This establishes a global, shared SSH connection.

    Args:
        remote_host  the catalog provider to connect to
    """
    global _ssh  # pylint: disable=[global-statement]
    _ssh = SSHClient()
    _ssh.set_missing_host_key_policy(AutoAddPolicy())
    _ssh.connect(remote_host, password='')


def close_ssh():
    """This closes the global, shared SSH connection."""
    _ssh.close()  # noqa: F821


def tail_rods_log(num_lines: int = 0) -> list[str]:
    """
    Reads the last part of the rodsLog on the connected iRODS server.

    Parameters:
        num_lines  the number of lines to read

    Returns:
        the last num_lines of the log file
    """
    if num_lines <= 0:
        lines = "+1"
    else:
        lines = f"{num_lines}"
    _, stdout, _ = _ssh.exec_command(f'tail --lines={lines} /var/lib/irods/log/rodsLog.*')  # noqa: E501, F821  # pylint: disable=[line-too-long]
    return stdout.read().decode().splitlines()


class IrodsType(Enum):
    """This class encodes values as a given iRODS type for passing to an iRODS rule."""

    BOOLEAN = enum.auto()
    NONE = enum.auto()
    PATH = enum.auto()
    STRING = enum.auto()
    STRING_LIST = enum.auto()

    def format(self, value: any) -> Optional[str]:
        """
        formats a value for iRODS rule base on its type

        Parameters:
            value  the value to format

        Returns:
            the formatted value
        """
        if value is None:
            return None
        if self == IrodsType.BOOLEAN:
            return str(value).lower()
        if self == IrodsType.NONE:
            return None
        if self == IrodsType.PATH:
            return str(value)
        if self == IrodsType.STRING:
            return f'"{value}"'
        return 'list(' + ','.join(map(IrodsType.STRING.format, value)) + ')'


class IrodsVal:
    """This holds a value to pass into an iRODS rule."""

    @staticmethod
    def boolean(val: bool):
        """
        Construct an iRODS Boolean.

        Parameters:
            val  a Boolean value

        Return:
            An _IrodsVal of type _IrodsType.BOOL
        """
        return IrodsVal(IrodsType.BOOLEAN, val)

    @staticmethod
    def none():
        """
        Construct an empty result

        Return:
            An _IrodsVal of type _IrodsType.NONE
        """
        return IrodsVal(IrodsType.NONE, None)

    @staticmethod
    def path(path: str):
        """
        Construct an iRODS path.

        Parameters:
            path  the value of the path to construct

        Return:
            An _IrodsVal of type _IrodsType.PATH
        """
        return IrodsVal(IrodsType.PATH, path)

    @staticmethod
    def string(val: str):
        """
        Construct an iRODS string.

        Parameters:
            val  the value of the string to construct

        Return:
            An _IrodsVal of type _IrodsType.STRING
        """
        return IrodsVal(IrodsType.STRING, val)

    @staticmethod
    def string_list(val: list[str]):
        """
        Construct an iRODS list of strings

        Parameters:
            val  the python list of strings to convert

        Returns
            an _IrodsVal of type _IrodsType.STRING_LIST
        """
        return IrodsVal(IrodsType.STRING_LIST, val)

    def __init__(self, irods_type, val):
        self._type = irods_type
        self._irods_val = irods_type.format(val)

    def __eq__(self, other):
        return self.type == other.type and self._irods_val == other._irods_val

    def __repr__(self):
        return self._irods_val

    @property
    def type(self) -> IrodsType:
        """The iRODS type of the value"""
        return self._type


class _RuleExecFailure(Exception):
    pass


def unimplemented(test_case):
    """This decorator marks an test case as not being implemented. All tests will be skipped."""

    @classmethod
    def do_nothing(_):
        pass

    setattr(test_case, 'setUpClass', do_nothing)
    setattr(test_case, 'tearDownClass', do_nothing)
    setattr(test_case, 'setUp', do_nothing)
    setattr(test_case, 'tearDown', do_nothing)

    for attr in dir(test_case):
        if attr.startswith("test_"):
            delattr(test_case, attr)

    return test_case


class IrodsTestCase(TestCase):
    """This is a base class for all iRODS rule unit tests."""

    @staticmethod
    def prep_path(path: str) -> Tuple[IrodsVal.path, IrodsVal.string]:
        """
        This creates a pair of encoding for a path, one as an iRODS path type, and one as a iRODS
        string type.

        Args:
            path  the path to encode

        Returns:
            A tuple containing the two encodings.
        """
        return (IrodsVal.path(path), IrodsVal.string(path))

    def setUp(self):
        super().setUp()
        self._irods = iRODSSession(
            host=environ.get("IRODS_HOST"),
            port=int(environ.get("IRODS_PORT")),
            zone=environ.get("IRODS_ZONE_NAME"),
            user=environ.get("IRODS_USER_NAME"),
            password=environ.get("IRODS_PASSWORD"))

    def tearDown(self):
        self._irods.cleanup()
        return super().tearDown()

    @property
    def irods(self) -> iRODSSession:
        """provides access to an open iRODS session"""
        return self._irods

    def fn_test(self, fn: str, args: List[IrodsVal], exp_res: IrodsVal) -> None:
        """
        Tests an iRODS rule function.

        Parameters:
            fn       the iRODS rule function under test
            args     the list of input parameters to pass to the function
            exp_res  the expected result of the function call
        """
        rule = self._mk_rule(f"writeLine('stdout', {fn}({', '.join(map(repr, args))}))")
        try:
            self.assertEqual(self._exec_rule(rule, exp_res.type), exp_res)
        except _RuleExecFailure as ref:
            self.fail(str(ref))

    def _mk_rule(self, logic):
        return Rule(
            session=self._irods,
            instance_name='irods_rule_engine_plugin-irods_rule_language-instance',
            body=logic,
            output='ruleExecOut')

    def _exec_rule(self, rule, res_type):
        output = rule.execute(r_error=(r_errs := RErrorStack()))
        if r_errs:
            raise _RuleExecFailure(pprint.pformat([vars(r) for r in r_errs]))
        if not output or len(output.MsParam_PI) == 0:
            return IrodsVal.none()
        err_buf = output.MsParam_PI[0].inOutStruct.stderrBuf.buf
        if err_buf:
            raise _RuleExecFailure(err_buf.rstrip(b'\0').decode('utf-8'))
        buf = output.MsParam_PI[0].inOutStruct.stdoutBuf.buf
        return IrodsVal(res_type, buf.rstrip(b'\0').decode('utf-8').rstrip("\n"))
