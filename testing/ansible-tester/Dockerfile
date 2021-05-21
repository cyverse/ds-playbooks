FROM williamyeh/ansible:centos7

RUN yum --assumeyes \
      install bzip2-devel dmidecode gcc git libffi-devel openssl-devel postgresql rpm-build wget \
      https://files.renci.org/pub/irods/releases/4.1.10/centos7/irods-icommands-4.1.10-centos7-x86_64.rpm

ADD https://www.python.org/ftp/python/3.8.1/Python-3.8.1.tgz /root/

RUN tar --extract --directory /root --file /root/Python-3.8.1.tgz

WORKDIR /root/Python-3.8.1

RUN ./configure --enable-optimizations
RUN make altinstall

WORKDIR /

RUN python3.8 -m pip --disable-pip-version-check --quiet \
      install ansible dnspython netaddr python-irodsclient  requests urllib3 wheel
RUN mkdir --parents /inventory/group_vars /inventory/host_vars /playbooks-under-test /root/.ssh

COPY ansible.cfg /root/.ansible.cfg
COPY ssh-config /root/.ssh/config

COPY test-playbook.sh /test-playbook

RUN chmod --recursive go-rwx /root/.ssh
RUN chmod u+x /test-playbook

COPY inventory/* /inventory/
COPY inventory/host_vars/* /inventory/host_vars/
COPY inventory/group_vars/* /inventory/group_vars/
COPY wait-for-ready.yml /

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
