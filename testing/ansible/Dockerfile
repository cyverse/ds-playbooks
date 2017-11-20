FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

RUN mkdir --parents /inventory/group_vars /inventory/host_vars /root/.ssh

COPY ssh-config /root/.ssh/config

RUN chmod --recursive go-rwx /root/.ssh

COPY hosts.yml /inventory
COPY group_vars/* /inventory/group_vars/
COPY host_vars/* /inventory/host_vars/
COPY playbook.yml /

VOLUME /playbook-under-test.yml

ENTRYPOINT [ "ansible-playbook", "--inventory-file=/inventory", "/playbook.yml" ]
