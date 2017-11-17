# The production environment rule customizations belong in this file.

ipc_AMQP_URI = 'amqp://guest:guest@localhost:5672/%2Fprod%2Fdata-store'
ipc_AMQP_EPHEMERAL = false
ipc_IES_IP = '127.0.0.1'
ipc_RE_HOST = 'localhost'
ipc_RODSADMIN = 'rods'

ipc_DEFAULT_RESC = 'demoResc'


acSetNumThreads {
  ON($userClientName == 'ipctest' || $userClientName == 'rhg') {
    msiSetNumThreads('default', '16', 'default');
  }
}

