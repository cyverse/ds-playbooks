FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

RUN mkdir --parents /root/.ssh

COPY ssh-config /root/.ssh/config

RUN chmod --recursive go-rwx /root/.ssh

COPY inventory playbook.yml /

VOLUME /playbook-under-test.yml

ENTRYPOINT [ "ansible-playbook", "--inventory-file=/inventory", "/playbook.yml" ]
