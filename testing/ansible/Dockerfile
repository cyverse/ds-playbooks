FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

VOLUME /inventory /playbook.yml

ENTRYPOINT [ "/usr/bin/ansible-playbook", "--inventory-file=/inventory", "/playbook.yml" ]
