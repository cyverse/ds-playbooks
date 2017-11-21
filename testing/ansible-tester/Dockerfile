FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

RUN yum --assumeyes install python-dns && \
    yum clean all && \
    rm --force --recursive /var/cache/yum && \
    mkdir --parents /inventory/group_vars /inventory/host_vars /playbooks-under-test /root/.ssh

COPY ansible.cfg /root/.ansible.cfg
COPY ssh-config /root/.ssh/config

RUN chmod --recursive go-rwx /root/.ssh

COPY inventory/hosts.yml /inventory
COPY inventory/host_vars/dbms /inventory/host_vars
COPY inventory/group_vars/* /inventory/group_vars/
COPY wait-for-ready.yml /
COPY test-playbook.sh /test-playbook

VOLUME /playbooks-under-test

ENTRYPOINT [ "/test-playbook" ]
