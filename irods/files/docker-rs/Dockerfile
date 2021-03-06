FROM cyverse/irods-rs:4.1.10

### Switch back to root for installation
USER root

### Install Set AVU plugin
COPY irods-setavu-plugin/libraries/centos7/libmsiSetAVU.so /var/lib/irods/plugins/microservices

### Install iRODS NetCDF plugins
COPY irods-netcdf-build/packages/centos7/*.rpm /tmp/
RUN yum --assumeyes install \
        https://files.renci.org/pub/irods/releases/4.1.10/centos7/irods-runtime-4.1.10-centos7-x86_64.rpm && \
    yum --assumeyes install \
        /tmp/irods-api-plugin-netcdf-1.0-centos7.rpm \
        /tmp/irods-icommands-netcdf-1.0-centos7.rpm \
        /tmp/irods-microservice-plugin-netcdf-1.0-centos7.rpm && \
### Install support for UUID generation
    yum --assumeyes install uuidd && \
### Install support for periphery script
    yum --assumeyes install moreutils && \
### Clean up installation artifacts
    yum --assumeyes clean all && \
    rm --force --recursive /var/cache/yum /tmp/* && \
### Create vault base
    mkdir /irods_vault

### Install cmd scripts
COPY cmd/* /var/lib/irods/iRODS/server/bin/cmd/
RUN chown irods:irods /var/lib/irods/iRODS/server/bin/cmd/* && \
    chmod ug+x /var/lib/irods/iRODS/server/bin/cmd/*

### Install iRODS configuration files
COPY etc/irods/* /etc/irods/
COPY var/lib/irods/.irods/irods_environment.json /var/lib/irods/.irods
RUN chown irods:irods /etc/irods/* /var/lib/irods/.irods/irods_environment.json && \
    chmod -R ug+r /var/lib/irods && \
### Ensure /etc/irods/, .irods/, and log/ are group writeable
    chmod -R ug+rw /etc/irods/ /var/lib/irods/.irods /var/lib/irods/iRODS/server/log

### Add script to handle start and stop extras
COPY periphery.sh /periphery
RUN chown irods:irods /periphery && \
    chmod ug+x /periphery

VOLUME /var/lib/irods/iRODS/server/log /var/lib/irods/iRODS/server/log/proc

EXPOSE 1247/tcp 1248/tcp 20000-20009/tcp 20000-20009/udp

ENV IRODS_CONTROL_PLANE_KEY=TEMPORARY__32byte_ctrl_plane_key
ENV IRODS_NEGOTIATION_KEY=TEMPORARY_32byte_negotiation_key
ENV IRODS_ZONE_KEY=TEMPORARY_zone_key

CMD [ "/periphery" ]

### Prepare onbuild instantiation logic
COPY on-build-instantiate.sh /on-build-instantiate
RUN chmod u+x /on-build-instantiate

ONBUILD ARG IRODS_CLERVER_USER=ipc_admin
ONBUILD ARG IRODS_DEFAULT_RES=CyVerseRes
ONBUILD ARG IRODS_HOST_UID
ONBUILD ARG IRODS_RES_SERVER
ONBUILD ARG IRODS_STORAGE_RES

ONBUILD RUN /on-build-instantiate && \
            rm --force /on-build-instantiate

ONBUILD VOLUME /irods_vault/"$IRODS_STORAGE_RES"

ONBUILD USER irods-host-user

ONBUILD ENV IRODS_STORAGE_RES="$IRODS_STORAGE_RES"
