FROM cyverse/irods:4.2.8

### Switch back to root for installation
USER root

### Install support for UUID generation
RUN yum --assumeyes install uuidd && \
### Install support for periphery script
    yum --assumeyes install moreutils && \
### Clean up installation artifacts
    yum --assumeyes clean all && \
    rm --force --recursive /var/cache/yum && \
### Create vault base
    mkdir /irods_vault

### Install cmd scripts and config files
COPY var/lib/irods/ /var/lib/irods/
COPY etc/irods/ /etc/irods/
RUN chown --recursive irods:irods /etc/irods /var/lib/irods && \
    chmod --recursive ug+rw /etc/irods /var/lib/irods && \
    chmod ug+x /var/lib/irods/irodsctl /var/lib/irods/msiExecCmd_bin/*

### Add script to handle start and stop extras
COPY periphery.sh /periphery
RUN chown irods:irods /periphery && \
    chmod ug+x /periphery

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

ONBUILD USER irods-host-user
ONBUILD ENV HOME=/var/lib/irods

ONBUILD ENV IRODS_STORAGE_RES="$IRODS_STORAGE_RES"
