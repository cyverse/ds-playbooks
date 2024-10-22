"""
This lookup plugin generates UUIDs based on the provided type parameter.
"""

# Import the necessary Ansible libraries
from __future__ import absolute_import, division, print_function

# Import the UUID standard library
import uuid as impl  # pylint: disable=import-self

# Import base classes from Ansible
from ansible.plugins.lookup import LookupBase
from ansible.errors import AnsibleError

# Set the metaclass to ensure that the plugin is compatible with Ansible
__metaclass__ = type  # pylint: disable=invalid-name

DOCUMENTATION = r'''
---
module: uuid

short_description: Generate UUIDs based on the provided type parameter

version_added: "2.16.11"

description: >
  This lookup plugin generates UUIDs based on the provided type parameter.

  Supported types:
    - 1: Time-based UUID (UUID1)
    - 4: Random UUID (UUID4)
    - 3: Name-based UUID (UUID3 with MD5 hash)
    - 5: Name-based UUID (UUID5 with SHA1 hash)

options:
  _terms:
    description: >
      The first element must be the type of UUID to generate (1, 4, 3, or 5). For types 3 and 5, a
      namespace and name must also be provided as the second and third elements, respectively.
    required: True
    type: list
'''

EXAMPLES = r'''
---
- name: Generate a time-based UUID (UUID1)
  ansible.builtin.debug:
    msg: "{{ lookup('uuid', 1) }}"

- name: Generate a random UUID (UUID4)
  ansible.builtin.debug:
    msg: "{{ lookup('uuid', 4) }}"

- name: Generate a name-based UUID (UUID3 with MD5 hash)
  ansible.builtin.debug:
    msg: "{{ lookup('uuid', 3, '6ba7b810-9dad-11d1-80b4-00c04fd430c8', 'my_name') }}"
'''


class LookupModule(LookupBase):
    """ Generate UUIDs based on the provided type parameter """

    def run(self, terms, _variables=None, **_kwargs):
        """
        Generate UUIDs based on the provided type parameter.

        Supported types:
        - 1: Time-based UUID (UUID1)
        - 4: Random UUID (UUID4)
        - 3: Name-based UUID (UUID3 with MD5 hash)
        - 5: Name-based UUID (UUID5 with SHA1 hash)

        For name-based UUIDs (types 3 and 5), you must provide a namespace and a name:
        - 3, namespace, name
        - 5, namespace, name
        """

        # Ensure that at least one term is passed for the UUID type
        if len(terms) == 0:
            raise AnsibleError(
                "You must specify a UUID type: 1 (time-based), 4 (random), 3 (name-based UUID3),"
                " 5 (name-based UUID5)")

        uuid_type = int(terms[0])

        # Generate UUID based on the provided type using match statement
        match uuid_type:
            case 1:
                # Time-based UUID (UUID1)
                return [str(impl.uuid1())]  # pylint: disable=no-member

            case 4:
                # Random UUID (UUID4)
                return [str(impl.uuid4())]  # pylint: disable=no-member

            case 3:
                # UUID3 (MD5 hash)
                if len(terms) < 3:
                    raise AnsibleError(
                        "You must provide a namespace and a name for a name-based UUID (type 3)")
                return [str(impl.uuid3(impl.UUID(terms[1]), terms[2]))]  # pylint: disable=no-member

            case 5:
                # UUID5 (SHA1 hash)
                if len(terms) < 3:
                    raise AnsibleError(
                        "You must provide a namespace and a name for a name-based UUID (type 5)")
                return [str(impl.uuid5(impl.UUID(terms[1]), terms[2]))]  # pylint: disable=no-member

            case _:
                # If an unsupported UUID type is specified
                raise AnsibleError(
                    f"Unsupported UUID type '{uuid_type}'. Supported types: 1, 4, 3, 5"
                )
