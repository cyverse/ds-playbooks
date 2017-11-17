FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

COPY inventory /inventory
COPY playbook.yml /playbook.yml

ENTRYPOINT [ "ansible-playbook", "--inventory-file=/inventory", "/playbook.yml" ]
