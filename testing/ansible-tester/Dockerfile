FROM williamyeh/ansible:centos7

### Update system packages
RUN yum --assumeyes --quiet upgrade

### Install additional ansible support
RUN yum --assumeyes --quiet install dmidecode python3 rpm-build wget
# XXX - The latest version of pip (21.3.1) is breaking on CentOS 7 without
#       following encoding setup.
ENV LANG=en_US.UTF-8
# XXX - ^^^
RUN python3 -m pip --quiet install --upgrade pip
RUN python3 -m pip --disable-pip-version-check --quiet install \
  dnspython netaddr python-irodsclient requests urllib3 wheel
# NB: ansible-core 2.12+ requires Python 3.8, which can't be installed from the
#     epel on CentOS 7.
RUN python3 -m pip --disable-pip-version-check --quiet install 'ansible-core<2.12'

### Install inspection support
RUN yum --assumeyes install ca-certificates epel-release
RUN python -m pip --quiet uninstall --yes urllib3

ADD https://packages.irods.org/renci-irods.yum.repo /etc/yum.repos.d/renci-irods.yum.repo

RUN rpm --import https://packages.irods.org/irods-signing-key.asc
RUN yum --assumeyes --quiet install irods-icommands-4.2.8 postgresql
RUN mkdir /root/.ssh

COPY ssh-config /root/.ssh/config
RUN chmod --recursive go-rwx /root/.ssh

ARG IRODS_CLERVER_PASSWORD=rods

ENV IRODS_HOST=localhost
ENV IRODS_PASSWORD=$IRODS_CLERVER_PASSWORD
ENV IRODS_PORT=1247
ENV IRODS_USER_NAME=rods
ENV IRODS_ZONE_NAME=tempZone
ENV PGDATABASE=ICAT
ENV PGHOST=localhost
ENV PGPASSWORD=testpassword
ENV PGUSER=irods

# XXX - Due to a bug in iCommands 4.2.8, irods_environment.json needs to exist
#       prior to `iinit` execution.
RUN mkdir /root/.irods
RUN echo '{}' > /root/.irods/irods_environment.json
# XXX - ^^^

### Configure testing infrastructure
RUN mkdir --parents /inventory/group_vars /inventory/host_vars /playbooks-under-test

COPY ansible.cfg /root/.ansible.cfg
COPY requirements.yml /requirements.yml

RUN ansible-galaxy collection install --requirements-file=/requirements.yml

COPY inventory/* /inventory/
COPY inventory/host_vars/* /inventory/host_vars/
COPY inventory/group_vars/* /inventory/group_vars/
COPY wait-for-ready.yml /

VOLUME /playbooks-under-test

COPY test-playbook.sh /test-playbook
RUN chmod u+x /test-playbook

ENTRYPOINT [ "/test-playbook" ]
