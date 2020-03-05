# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.


## Prerequisites

These playbooks run docker and git on the control machine, so they need to be installed. Also the
user executing ansible needs to be a member of the docker group.

If the control machine runs linux, `dmidecode` needs to be installed for `gather_facts` to work.

`rpm-build` (using `yum` or `rpmbuild` if `apt-get`) needs to be installed on control machine for
`irods/tasks/build_dummy_rpm.yml` to work.

The following python packages are required on the control machine for `docker_image_info` to work:
`docker-py`, `requests`, and `urllib3`. They should be installed using `pip`.
