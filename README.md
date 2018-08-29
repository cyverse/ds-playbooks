# CyVerse DS Playbooks

This is a collection of playbooks for maintaining CyVerse's Data Store.


## Prerequisites

These playbooks run docker and git on the control machine, so they need to be
installed. Also the user executing ansible needs to be a member of the docker
group.

If the control machine runs linux, `dmidecode` needs to be installed for
`gather_facts` to work.
