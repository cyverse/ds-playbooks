FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive

### Update system packages
RUN apt-get --quiet --yes update
RUN apt-get --quiet --yes upgrade

RUN apt-get install --quiet --yes \
  dmidecode gpg-agent python-is-python3 python3 python3-pip openssh-client rpm wget

# NB: ansible-core==2.17.0 has a bug in it that prevents ansible.builtin.wait_for_connection from
# detecting CentOS 7 and Ubuntu 18.04 connection readiness.
RUN python3 -m pip --disable-pip-version-check --quiet install \
  'ansible-core<2.17' dnspython jinja2 netaddr 'python-irodsclient<2' requests urllib3 wheel

### Install inspection support
RUN apt-get install --quiet --yes ca-certificates

RUN wget --quiet --directory-prefix=/etc/apt/trusted.gpg.d \
  https://packages.irods.org/irods-signing-key.asc
RUN echo "deb [arch=amd64] https://packages.irods.org/apt/ bionic main" \
  > /etc/apt/sources.list.d/renci-irods.list
RUN apt-get --quiet --yes update
COPY apt-preferences-irods /etc/apt/preferences.d/irods

RUN echo "deb http://security.ubuntu.com/ubuntu focal-security main" \
  > /etc/apt/sources.list.d/focal-security.list
RUN apt-get --quiet --yes update
RUN apt-get --quiet --yes install libssl1.1

RUN mkdir irods-runtime
WORKDIR /irods-runtime
RUN wget \
  https://packages.irods.org/apt/pool/bionic/main/i/irods-runtime/irods-runtime_4.2.8_amd64.deb
RUN ar x irods-runtime_4.2.8_amd64.deb
RUN tar xzf control.tar.gz
RUN sed --in-place '/^Depends:/s/ libssl1.0.0,/ libssl1.1,/' control
RUN sed --in-place '/^Depends:/s/ python/ python3/g' control
RUN tar c control md5sums | gzip -c > control.tar.gz
RUN ar rcs irods-runtime_hack.deb debian-binary control.tar.gz data.tar.gz
RUN apt-get install --quiet --yes ./irods-runtime_hack.deb
WORKDIR /

RUN mkdir irods-icommands
WORKDIR /irods-icommands
RUN wget \
  https://packages.irods.org/apt/pool/bionic/main/i/irods-icommands/irods-icommands_4.2.8_amd64.deb
RUN ar x irods-icommands_4.2.8_amd64.deb
RUN tar xzf control.tar.gz
RUN sed --in-place '/^Depends:/s/ libssl1.0.0$/ libssl1.1/' control
RUN tar c control md5sums | gzip -c > control.tar.gz
RUN ar rcs irods-icommands_hack.deb debian-binary control.tar.gz data.tar.gz
RUN apt-get install --quiet --yes ./irods-icommands_hack.deb
WORKDIR /

RUN apt-get install --quiet --yes postgresql

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
COPY ansible.cfg /root/.ansible.cfg

COPY requirements.yml /requirements.yml
RUN ansible-galaxy collection install --requirements-file=/requirements.yml

COPY inventory /inventory
COPY wait-for-ready.yml /
COPY --chmod=100 test-playbook.sh /test-playbook

RUN mkdir /playbooks-under-test
VOLUME /playbooks-under-test

ENTRYPOINT [ "/test-playbook" ]
