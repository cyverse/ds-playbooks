# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.


## Prerequisites

For CentOS machines, the Docker-CE yum repository needs to be set up.

```bash
yum-config-manager --add-repo https://download.docker.com/linux/centos/docker-ce.repo
```

The following system packages need to be installed on control machine.

* `dmidecode` (if control machine runs linux)
* `docker-ce`
* `python3`
* `python3-pip`
* `rpm-build` (`rpm`) if control machine runs Debian or Ubuntu)

The following python packages need to be installed on the control machine using `python3 -m pip`.

* `ansible`
* `docker`
* `dnspython`
* `netaddr`
* `python-irodsclient`
* `requests`
* `urllib3`
* `wheel`

The user executing ansible needs to be a member of the `docker` group.
