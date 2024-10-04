"""
This lookup plugin generates UUIDs based on the provided type parameter.
"""

# Import the necessary Ansible libraries
from __future__ import absolute_import, division, print_function
__metaclass__ = type #pylint: disable=invalid-name

DOCUMENTATION = '''
module: uuid

short_description: Generate UUIDs based on the provided type parameter

version_added: "1.0.0"

description: >
    This lookup plugin generates UUIDs based on the provided type parameter.
    Supported types:
    - 1: Time-based UUID (UUID1)
    - 4: Random UUID (UUID4)
    - 3: Name-based UUID (UUID3 with MD5 hash)
    - 5: Name-based UUID (UUID5 with SHA1 hash)

options:
    _terms:
        description: The type of UUID to generate (1, 4, 3, or 5). For types 3 and 5, a namespace and name must also be provided.
        required: True
        type: list
        elements: str
'''
EXAMPLES = '''
- name: Generate a time-based UUID (UUID1)
    debug:
        msg: "{{ lookup('uuid', 1) }}"

- name: Generate a random UUID (UUID4)
    debug:
        msg: "{{ lookup('uuid', 4) }}"

- name: Generate a name-based UUID (UUID3 with MD5 hash)
    debug:
        msg: "{{ lookup('uuid', 3, '6ba7b810-9dad-11d1-80b4-00c04fd430c8', 'my_name') }}"
'''

# Import the UUID standard library
import uuid #pylint: disable=import-self disable=wrong-import-position

# Import base classes from Ansible
from ansible.plugins.lookup import LookupBase #pylint: disable=wrong-import-position
from ansible.errors import AnsibleError #pylint: disable=wrong-import-position


class LookupModule(LookupBase):
    """ Generate UUIDs based on the provided type parameter """
    def run(self, terms, variables=None, **kwargs): #pylint: disable=unused-argument
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
            "You must specify a UUID type: 1 (time-based), \
             4 (random), 3 (name-based UUID3), 5 (name-based UUID5)"
            ) #pylint: disable=trailing-whitespace

        uuid_type = int(terms[0])

        # Generate UUID based on the provided type using match statement
        match uuid_type:
            case 1:
                # Time-based UUID (UUID1)
                return [str(uuid.uuid1())]

            case 4:
                # Random UUID (UUID4)
                return [str(uuid.uuid4())]

            case 3:
                # UUID3 (MD5 hash)
                if len(terms) < 3:
                    raise AnsibleError(
                    "You must provide a namespace and a name for a name-based UUID (type 3)"
                    )
                return [str(uuid.uuid3(uuid.UUID(terms[1]), terms[2]))]

            case 5:
                # UUID5 (SHA1 hash)
                if len(terms) < 3:
                    raise AnsibleError(
                    "You must provide a namespace and a name for a name-based UUID (type 5)"
                    )
                return [str(uuid.uuid5(uuid.UUID(terms[1]), terms[2]))]

            case _:
                # If an unsupported UUID type is specified
                raise AnsibleError(
                    f"Unsupported UUID type '{uuid_type}'. Supported types: 1, 4, 3, 5"
                )
