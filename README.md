# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.


## Prerequisites

The following system packages need to be installed on control machine.

* `ansible`
* `dmidecode` (if control machine runs linux)
* `docker-ce`
* `git`
* `jq`
* `python-dns`
* `python-netaddr`
* `python-pip`
* `rpm-build` (`rpm` if control machine runs Debian or Ubuntu)

The following python packages need to be installed on the control machine using `pip`.

* `docker`
* `python-irodsclient`
* `requests`
* `urllib3`

The user executing ansible needs to be a member of the `docker` group.
