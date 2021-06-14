FROM cyverse/irods:4.2.8

### Switch back to root for installation
USER root

### Install NetCDF plugins
RUN yum --assumeyes install \
      http://people.renci.org/~dmoore/irods_netcdf/packages_2021_03_24/irods-netcdf-client_modules-4.2.8.0-centos-7-x86_64.rpm \
      http://people.renci.org/~dmoore/irods_netcdf/packages_2021_03_24/irods-netcdf-icommands-4.2.8.0-centos-7-x86_64.rpm \
      http://people.renci.org/~dmoore/irods_netcdf/packages_2021_03_24/irods-netcdf-server_modules-4.2.8.0-centos-7-x86_64.rpm && \
### Install support for UUID generation
    yum --assumeyes install uuidd && \
### Install support for periphery script
    yum --assumeyes install moreutils && \
### Clean up installation artifacts
    yum --assumeyes clean all && \
    rm --force --recursive /var/cache/yum && \
### Create vault base
    mkdir /irods_vault

### Install cmd scripts
COPY cmd/* /var/lib/irods/msiExecCmd_bin/
RUN chown irods:irods /var/lib/irods/msiExecCmd_bin/*

### Install iRODS configuration files
COPY etc/irods/* /etc/irods/
COPY var/lib/irods/.irods/irods_environment.json /var/lib/irods/.irods
RUN chown irods:irods /etc/irods/* /var/lib/irods/.irods/irods_environment.json

### Convert configuration files to 4.2.8 style
USER irods
COPY upgrade_config_files.py /var/lib/irods/scripts/
RUN python /var/lib/irods/scripts/upgrade_config_files.py && \
    jq \
        '.plugin_configuration.rule_engines += \
          [ \
            { \
              "instance_name": "irods_rule_engine_plugin-cpp_default_policy-instance", \
              "plugin_name": "irods_rule_engine_plugin-cpp_default_policy", \
              "plugin_specific_configuration": {} \
            } \
          ]' \
        /etc/irods/server_config.json \
      | sponge /etc/irods/server_config.json
    rm --force /etc/irods/*.prev.* /var/lib/irods/.irods/irods_environment.json.prev.* && \
    rm --force /var/lib/irods/scripts/upgrade_config_files.py
USER root

### Add script to handle start and stop extras
COPY periphery.sh /periphery
RUN chown irods:irods /periphery

### Ensure correct permissions
RUN chmod -R ug+rw /etc/irods /var/lib/irods /var/lib/irods/.irods && \
    chmod ug+x /periphery /var/lib/irods/irodsctl /var/lib/irods/msiExecCmd_bin/*

VOLUME /var/lib/irods/log /var/lib/irods/log/proc

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
