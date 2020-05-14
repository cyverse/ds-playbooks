#!/usr/bin/python
"""
Provides an ansible module for creating and removing iRODS users.
"""

import ssl
from ansible.module_utils.basic import AnsibleModule

ANSIBLE_METADATA = {
    "metadata_version": "1.1",
    "status": ["preview"],
    "supported_by": "community"
}

DOCUMENTATION = """
---
module: irods_user

short_description: Create/Remove users

version_added: "2.4"

description:
    - "Create/Remove iRODS user"

options:
    users:
        description:
            - Username of users
        required: true
    state:
        description:
            - Desired state to achieve
            - Either present or absent
        required: true
    host:
        description:
            - Hostname of the iRODS server
        required: true
    port:
        description:
            - Port of the iRODS server
        required: true
    admin_user:
        description:
            - Username of the admin user
        required: true
    admin_password:
        description:
            - Password of the admin user
        required: true
    zone:
        description:
            - Zone of the admin user
        required: true

author:
    - John Xu
"""

EXAMPLES = '''
# Create iRODS users
- name: create users
  irods_user:
    users:
        - test_user1
        - test_user2
    state: present
    host: cyverse.org
    port: 1247
    admin_user: rods
    admin_password: 1234
    zone: tempZone

# Remove iRODS users
- name: remove users
  irods_user:
    users:
        - test_user1
        - test_user2
    state: absent
    host: cyverse.org
    port: 1247
    admin_user: rods
    admin_password: 1234
    zone: tempZone
'''

RETURN = '''
message:
    description: Performed operation
    type: str
    returned: always
users:
    description: List of users that has been changed, empty list if none
    type: list
    returned: always
'''

try:
    USE_IRODS_CLIENT = True
    from irods.session import iRODSSession
    from irods.models import User
except ImportError:
    USE_IRODS_CLIENT = False


class IRODSUserModule:
    """
    Module class
    """

    def __init__(self):
        """
        Initialize the module
        """
        # define argument
        self.module_args = dict(
            users=dict(type="list", elements="str", required=True),
            state=dict(type="str", required=True,
                       choices=["present", "absent"]),

            host=dict(type="str", required=True),
            port=dict(type="int", required=True),
            admin_user=dict(type="str", no_log=True, required=True),
            admin_password=dict(type="str", no_log=True, required=True),
            zone=dict(type="str", required=True),
        )
        # result
        self.result = dict(
            changed=False,
            message="",
            users=[]
        )

        # init module
        self.module = AnsibleModule(
            argument_spec=self.module_args,
            supports_check_mode=True
        )

        self.session = None

    def run(self):
        """
        Entry point for module class, method to be called to run the module
        """
        # check param and env
        self.sanity_check()

        # only-check mode
        if self.module.check_mode:
            self.module.exit_json(**self.result)

        self.init_session()

        action = self.select_action()
        action()

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

    def _success(self, msg=""):
        """
        Success routine, called when the operation succeeds
        """
        if msg:
            self.result["message"] = msg
        self.module.exit_json(**self.result)

    def sanity_check(self):
        """
        Check if python-irodsclient is installed
        """
        # python-irodsclient is required at this point
        if not USE_IRODS_CLIENT:
            self._fail("python-irodsclient not installed")

    def init_session(self):
        """
        Initialize the iRODS session with iRODS server
        """
        ssl_context = ssl.create_default_context(
            purpose=ssl.Purpose.SERVER_AUTH, cafile=None, capath=None,
            cadata=None)
        ssl_settings = {"ssl_context": ssl_context}
        self.session = iRODSSession(
            host=self.module.params["host"],
            port=self.module.params["port"],
            user=self.module.params["admin_user"],
            password=self.module.params["admin_password"],
            zone=self.module.params["zone"],
            **ssl_settings)

    def select_action(self):
        """
        Dispatch action according to the argument passed to the module
        """
        if self.module.params["state"] == "present":
            return self.user_present
        return self.user_absent

    def user_present(self):
        """
        Ensure list of users specified in the parameter are present
        """
        # get set of users
        users = set(self.module.params["users"])
        if not users:
            self._success("no user are specified")

        # check if users exist, select the non-exist ones
        users = {user for user in users if not self._user_exist(user)}

        # create users
        for user in users:
            self._create_user(user)
            self.result["users"].append(user)

        # check if users have being added
        vanished_users = {user for user in users if not self._user_exist(user)}
        if vanished_users:
            self._fail("users disappear after creation, {}".format(
                vanished_users))
        self._success()

    def user_absent(self):
        """
        Ensure list of users specified in the parameter are absent
        """
        # get set of users
        users = set(self.module.params["users"])
        if not users:
            self._success("no user are specified")

        # check if user exist, select the exist ones
        users = {user for user in users if self._user_exist(user)}

        # remove users
        for user in users:
            self._remove_user(user)
            self.result["users"].append(user)

        # check if users have been removed
        leftover_users = {user for user in users if self._user_exist(user)}
        if leftover_users:
            self._fail("users still exist after removal, {}".format(
                leftover_users))

    def _create_user(self, username):
        """
        Create an iRODS user with the given username
        """
        try:
            self.session.users.create(self, username, "rodsuser")
            self.result["changed"] = True
        except Exception as exc:
            # A broad catch on all exception type that could be raised by the
            # call to irods module, since the possible exception types are
            # not well documented.
            self._fail("Unable to create user {}".format(username), exc)

    def _remove_user(self, username):
        """
        Remove the iRODS user with the given username
        """
        try:
            self.session.users.remove(self, username)
            self.result["changed"] = True
        except Exception as exc:
            # A broad catch on all exception type that could be raised by the
            # call to irods module, since the possible exception types are
            # not well documented.
            self._fail("Unable to remove user {}".format(username), exc)

    def _user_exist(self, username):
        """
        Check if there exist an iRODS user with the given username
        """
        try:
            query = self.session.query(User.name)

            for result in query:
                if username == result[User.name]:
                    return True
            return False
        except Exception as exc:
            # A broad catch on all exception type that could be raised by the
            # call to irods module, since the possible exception types are
            # not well documented.
            self._fail("Unable to query irods user {}".format(username), exc)


def main():
    """
    Entrypoint of the Ansible module
    """
    module = IRODSUserModule()
    module.run()


if __name__ == '__main__':
    main()
