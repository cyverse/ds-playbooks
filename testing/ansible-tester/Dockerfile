FROM williamyeh/ansible:centos7
MAINTAINER Tony Edgin @ CyVerse

RUN yum --assumeyes install \
        git postgresql python-dns wget \
        ftp://ftp.renci.org/pub/irods/releases/4.1.10/centos7/irods-icommands-4.1.10-centos7-x86_64.rpm && \
    yum clean all && \
    rm --force --recursive /var/cache/yum && \
    mkdir --parents /inventory/group_vars /inventory/host_vars /playbooks-under-test /root/.ssh

COPY ansible.cfg /root/.ansible.cfg
COPY ssh-config /root/.ssh/config

RUN chmod --recursive go-rwx /root/.ssh

COPY inventory/hosts.yml /inventory
COPY inventory/host_vars/* /inventory/host_vars/
COPY inventory/group_vars/* /inventory/group_vars/
COPY wait-for-ready.yml /
COPY test-playbook.sh /test-playbook

VOLUME /playbooks-under-test

ENV IRODS_HOST=ies
ENV IRODS_PASSWORD=rods
ENV IRODS_PORT=1247
ENV IRODS_USER_NAME=rods
ENV IRODS_ZONE_NAME=tempZone
ENV PGDATABASE=ICAT
ENV PGHOST=dbms
ENV PGPASSWORD=testpassword
ENV PGUSER=irods

ENTRYPOINT [ "/test-playbook" ]
