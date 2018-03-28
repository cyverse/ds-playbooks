FROM centos:6
MAINTAINER Tony Edgin @ CyVerse

COPY ansible-support/config.sh /tmp/ansible-support
COPY ies/scripts/* /tmp/

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_HOST=dbms
ARG DBMS_PORT=5432
ARG DBMS_TYPE=postgres
ARG IRODS_CONTROL_PLANE_KEY=TEMPORARY__32byte_ctrl_plane_key
ARG IRODS_CONTROL_PLANE_PORT=1248
ARG IRODS_DEFAULT_RESOURCE=demoResc
ARG IRODS_FIRST_EPHEMERAL_PORT=20000
ARG IRODS_HOST=ies
ARG IRODS_LAST_EPHEMERAL_PORT=20199
ARG IRODS_NEGOTIATION_KEY=TEMPORARY_32byte_negotiation_key
ARG IRODS_SCHEMA_VALIDATION=https://schemas.irods.org/configuration
ARG IRODS_SYSTEM_GROUP=irods
ARG IRODS_SYSTEM_USER=irods
ARG IRODS_ZONE_KEY=TEMPORARY_zone_key
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_PORT=1247
ARG IRODS_ZONE_USER=rods

RUN /tmp/ansible-support 6 && \
    yum --assumeyes install \
      https://download.postgresql.org/pub/repos/yum/9.3/redhat/rhel-6-x86_64/pgdg-centos93-9.3-3.noarch.rpm && \
    yum --assumeyes install \
      ftp://ftp.renci.org/pub/irods/releases/4.1.10/centos6/irods-icat-4.1.10-centos6-x86_64.rpm \
      ftp://ftp.renci.org/pub/irods/releases/4.1.10/centos6/irods-database-plugin-postgres93-1.10-centos6-x86_64.rpm && \
    /tmp/config.sh && \
    pip install pika && \
    yum clean all && \
    rm --force --recursive /tmp/* /var/cache/yum

EXPOSE "$IRODS_ZONE_PORT"/tcp \
       "$IRODS_FIRST_EPHEMERAL_PORT"-"$IRODS_LAST_EPHEMERAL_PORT"/tcp \
       "$IRODS_FIRST_EPHEMERAL_PORT"-"$IRODS_LAST_EPHEMERAL_PORT"/udp

VOLUME /etc/irods /var/lib/irods/iRODS/server/log /var/lib/irods/.irods

COPY ansible-support/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
CMD [ "/service.sh" ]
