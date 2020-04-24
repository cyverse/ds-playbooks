FROM test-env-base:centos6

RUN touch /etc/sysconfig/iptables
RUN gpg --keyserver pool.sks-keyservers.net --recv-keys B42F6819007F00F88E364FD4036A9C25BF357DD4

ADD https://github.com/tianon/gosu/releases/download/1.12/gosu-amd64 /usr/local/bin/gosu
ADD https://github.com/tianon/gosu/releases/download/1.12/gosu-amd64.asc /usr/local/bin/gosu.asc

RUN gpg --verify /usr/local/bin/gosu.asc
RUN chmod +x /usr/local/bin/gosu
RUN yum install --assumeyes \
      https://download.postgresql.org/pub/repos/yum/9.3/redhat/rhel-6-x86_64/pgdg-centos93-9.3-3.noarch.rpm
RUN yum install --assumeyes postgresql93-server

ENV PATH "$PATH":/usr/pgsql-9.3/bin
ENV PGDATA=/var/lib/pgsql/9.3/data

RUN gosu postgres initdb --auth ident --lc-collate C --locale en_US.UTF-8

COPY postgresql/config/* "$PGDATA"/
RUN chown postgres:postgres "$PGDATA"/*

COPY postgresql/scripts/*.sql* /tmp/
COPY postgresql/scripts/config.sh /tmp/config

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_PORT=5432
ARG IRODS_RESOURCES=demoResc:rs:/var/lib/irods/Vault
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_USER=rods

RUN gosu postgres /tmp/config

COPY postgresql/service.sh /

CMD [ "/service.sh" ]
