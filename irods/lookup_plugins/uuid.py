# Import the necessary Ansible libraries
from __future__ import absolute_import, division, print_function
__metaclass__ = type

DOCUMENTATION = '''
module: uuid

short_description: Generate UUIDs based on the provided type parameter

version: 0.1

description: >
    This module generates UUIDs for the user based on the provided type parameter.
    Supported types:
    - 't': Time-based UUID (UUID1)
    - 'r': Random UUID (UUID4)
    - 'n': Name-based UUID (UUID3 for MD5 or UUID5 for SHA1, based on optional hashing method parameter)
'''
EXAMPLES = ''''''
# Import the UUID standard library
import uuid

# Import base classes from Ansible
from ansible.plugins.lookup import LookupBase
from ansible.errors import AnsibleError

class LookupModule(LookupBase):

    def run(self, terms, variables=None, **kwargs):
        """
        Generate UUIDs based on the provided type parameter.
        
        Supported types:
        - 1: Time-based UUID (UUID1)
        - 4: Random UUID (UUID4)
        - 3: UUID3 (MD5 hash) 
        - 5: UUID5 (SHA1 hash)
        
        For name-based UUIDs (type 'n'), you must provide a namespace and a name:
        - 'n', namespace, name, [optional: hash method ('md5' or 'sha1')]
        
        Example usage in a playbook:
        - debug: msg="{{ lookup('uuid', 1)}}"  # Time-based UUID
        - debug: msg="{{ lookup('uuid', 3, '6ba7b810-9dad-11d1-80b4-00c04fd430c8') }}"
        """

        # Ensure that at least one term is passed for the UUID type
        if len(terms) == 0:
            raise AnsibleError("You must specify a UUID type: 't' (time-based), 'r' (random), 'n' (name-based)")

        uuid_type = int(terms[0])

        # Generate UUID based on the provided type
        if uuid_type == 1:
            # Time-based UUID (UUID1)
            return [str(uuid.uuid1())]

        elif uuid_type == 4:
            # Random UUID (UUID4)
            return [str(uuid.uuid4())]

        elif uuid_type == 3:
            # UUID3 (MD5 hash)
            if len(terms) < 3:
                raise AnsibleError("You must provide a namespace and a name for a name-based UUID (type 'n')")
            return [str(uuid.uuid3(uuid.UUID(terms[1]), terms[2]))]
        elif uuid_type == 5:
            # UUID5 (SHA1 hash)
            if len(terms) < 3:
                raise AnsibleError("You must provide a namespace and a name for a name-based UUID (type 'n')")
            return [str(uuid.uuid5(uuid.UUID(terms[1]), terms[2]))]

        else:
            # If an unsupported UUID type is specified
            raise AnsibleError(f"Unsupported UUID type '{uuid_type}'. Supported types: 't', 'r', 'n'")
