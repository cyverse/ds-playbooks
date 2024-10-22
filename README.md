# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.

## Prerequisites

Only Ubuntu 22.04 is supported at this time.

The following actions need to be performed once for the admin host.

1. The Docker package repository needs to be configured on development machines and Ansible control nodes. Do the following.

   ```console
   sudo apt install ca-certificates curl gnupg lsb-release
   sudo mkdir /etc/apt/keyrings
   curl --fail --location --silent --show-error https://download.docker.com/linux/ubuntu/gpg \
      | sudo gpg --dearmor --output /etc/apt/keyrings/docker.gpg
   echo "deb [arch=$(dpkg --print-architecture) signed-by=/etc/apt/keyrings/docker.gpg] \
         https://download.docker.com/linux/ubuntu $(lsb_release --codename --short) stable" \
      | sudo tee /etc/apt/sources.list.d/docker.list
   sudo apt update
   ```

1. The following system packages need to be installed on development machines and Ansible control nodes.

   * dmidecode
   * docker-ce
   * docker-compose-plugin
   * git
   * jq
   * python3
   * python3-pip
   * rpm

   ```console
   sudo apt install \
      dmidecode docker-ce docker-compose-plugin git jq python3 python3-is-python python3-pip rpm
   ```

1. The docker service needs to be started.

   ```console
   sudo systemctl enable docker
   sudo systemctl start docker
   ```

The following actions need to be performed for each person who will be developing or deploying the Data Store.

1. The person needs to be added to the `docker` group. The following assumes the person's username is `DEVELOPER`.

   ```console
   sudo usermod --append --groups docker DEVELOPER
   ```

1. The following python packages need to be installed on the development machines and Ansible control nodes using `pip`.

   * ansible-core!=2.17.0
   * ansible-lint
   * dnspython
   * docker
   * molecule
   * molecule-plugins\[podman\]
   * netaddr
   * pika>1.2
   * python-irodsclient<2
   * wheel

   This is encapsulated in the file [requirements-python.txt](./requirements-python.txt).

   ```console
   pip install --requirement requirements-python.txt
   ```

   > [!NOTE]
   > Due to a bug in the version of `crun` that ships with Ubuntu 22.04, `podman` can't start systemd containers. See <https://noobient.com/2023/11/15/fixing-ubuntu-containers-failing-to-start-with-systemd/> for the work around.

1. Finally, the required ansible collections and roles need to be installed. This can be done by running the [init-ansible](./init-ansible) script.

   ```console
   ./init-ansible
   ```

> [!IMPORTANT]
> All VMs (including the Ansible Control Node, if that is a VM) shall install `rng-tools` using the playbook in `admin` directory called `install_rng_tools.yml`. This ensures that ansible tasks have efficient entropy in generating random numbers, preventing unexpected pauses in deployment.
