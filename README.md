# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.

## Prerequisites

The Docker package repository needs to be configured on development machines and Ansible control 
nodes.

For CentOS machines, do the following.

```console
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
```

For Ubuntu machines, do the following.

```console
apt update
apt install ca-certificates curl gnupg lsb-release
mkdir --parents /etc/apt/keyrings
curl --fail --location --silent --show-error https://download.docker.com/linux/ubuntu/gpg \
	| gpg --dearmor --output /etc/apt/keyrings/docker.gpg
```

The following system packages need to be installed on development machines and Ansible control 
nodes.

* dmidecode
* docker-ce
* python3
* python3-pip
* rpm-build (rpm) if control machine runs Debian or Ubuntu)

Docker Compose needs to be installed on development machines.

* docker-compose (CentOS)
* docker-compose-plugin (Ubuntu)

The following python packages need to be installed on the development machines and Ansible control
nodes using `pip`.

* ansible-core (This should be restricted to version 2.11)
* docker
* dnspython
* netaddr
* python-irodsclient
* requests
* urllib3
* wheel

The user executing ansible needs to be a member of the `docker` group.

Finally, the required ansible collections and roles need to be installed. This can be done by 
running the `init-ansible` script.