# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.

## Prerequisites

Only Ubuntu 22.04 is supported at this time.

The Docker package repository needs to be configured on development machines and Ansible control nodes. Do the following as the root user.

```console
prompt> apt update
prompt> apt install ca-certificates curl gnupg lsb-release
prompt> mkdir --parents /etc/apt/keyrings
prompt> curl --fail --location --silent --show-error https://download.docker.com/linux/ubuntu/gpg \
           | gpg --dearmor --output /etc/apt/keyrings/docker.gpg
prompt> echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
              https://download.docker.com/linux/ubuntu \
              $(lsb_release -cs) stable" \
           | tee /etc/apt/sources.list.d/docker.list
prompt> apt update
```

As the root user, the following system packages need to be installed on development machines and Ansible control nodes.

* dmidecode
* docker-ce
* docker-compose-plugin
* jq
* python3
* python3-pip
* rpm

The docker service needs to be started, and the developer needs to be a member of the `docker` group, and the docker service needs to be. Do the following as the root user where *DEVELOPER* is the username of the developer.

```console
prompt> systemctl enable docker
prompt> systemctl start docker
prompt> usermod --append --groups docker DEVELOPER
```

The file [requirements-python.txt](./requirements-python.txt) python packages need to be installed on the development machines and Ansible control nodes using `pip`.

* ansible-core!=2.17.0
* ansible-lint
* dnspython
* docker
* molecule
* molecule-plugins\[podman\]
* netaddr
* python-irodsclient<2
* wheel

Finally, the required ansible collections and roles need to be installed. This can be done by running the `init-ansible` script.

__XXX - crun on Ubuntu 22.04:__ Due to a bug in the version of crun that ships with Ubuntu 22.04, podman can't start systemd containers. See <https://noobient.com/2023/11/15/fixing-ubuntu-containers-failing-to-start-with-systemd/> for the work around.
