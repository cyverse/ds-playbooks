# -*- coding: utf-8 -*-

# Copyright: (c) 2022, The Arizona Board of Regents
# Standard BSD License | CyVerse (see https://cyverse.org/license)

"""This module provides the warn_if_false test."""

from ansible.utils.display import Display


DOCUMENTATION = r'''
---
name: warn_if_false

short_description: check Boolean value, if it is false, generate a warning

description: >
  This test checks a Boolean value. If the value is false, it will write
  warning message to the display.

version_added: "2.11.1"

author: Tony Edgin

options:
  _value:
    description: This is the value to test.
    required: true
    type: bool

  _warning:
    description: The warning message to display when _value is false.
    required: true
    type: str
'''

RETURN = r'''
_value:
  description: This is the value that was tested.
  type: bool
'''

EXAMPLES = r'''
- name: reboot server
  command: reboot
  async: 100
  poll: 0
  when: reboot_allowed is warn_if_false('reboot required')
'''


def _warn_if_false(_value: bool, _warning: str) -> bool:
    if not _value:
        Display().warning(_warning)
    return _value


class TestModule:  # noqa too-few-public-methods
    """Provided jinja2 tests."""

    def tests(self):  # noqa no-self-use
        """Provide tests."""
        return {
            'warn_if_false': _warn_if_false
        }
