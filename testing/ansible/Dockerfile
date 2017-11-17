FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

COPY inventory /inventory

VOLUME /playbook.yml

ENTRYPOINT [ "ansible-playbook", "--inventory-file=/inventory", "/playbook.yml" ]
