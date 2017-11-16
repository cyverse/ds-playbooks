FROM ansible-support:centos6
MAINTAINER Tony Edgin @ CyVerse

ENV PGDATA /var/lib/pgsql/9.3

RUN yum --assumeyes install \
      https://download.postgresql.org/pub/repos/yum/9.3/redhat/rhel-6-x86_64/pgdg-centos93-9.3-3.noarch.rpm && \
    yum --assumeyes install postgresql93-server && \
    yum clean all && \
    rm --force --recursive /var/cache/yum && \
    mkdir /data /logs && \
    chmod a+rwx /data /logs && \
    chown postgres:postgres /data /logs && \
    sed --in-place "s|^PGDATA=.*|PGDATA='$PGDATA'|" /var/lib/pgsql/.bash_profile && \
    su --command "/usr/pgsql-9.3/bin/initdb --auth ident \
                                            --lc-collate C \
                                            --locale en_US.UTF-8 \
                                            --pgdata /data" \
       --login postgres && \
    mv /data/*.conf "$PGDATA"/

COPY scripts/* /tmp/
COPY config/*.conf "$PGDATA"/

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_PORT=5432
ARG DBMS_TYPE=postgres
ARG IRODS_RESOURCES=demoResc:rs:/var/lib/irods/Vault
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_USER=rods

RUN chown postgres:postgres "$PGDATA"/* && \
    yum --assumeyes install cpp perl perl-JSON && \
    su --command "export DB_NAME='$DB_NAME' && \
                  export DB_PASSWORD='$DB_PASSWORD' && \
                  export DB_USER='$DB_USER' && \
                  export DBMS_PORT='$DBMS_PORT' && \
                  export DBMS_TYPE='$DBMS_TYPE' && \
                  export IRODS_RESOURCES='$IRODS_RESOURCES' && \
                  export IRODS_ZONE_NAME='$IRODS_ZONE_NAME' && \
                  export IRODS_ZONE_PASSWORD='$IRODS_ZONE_PASSWORD' && \
                  export IRODS_ZONE_USER='$IRODS_ZONE_USER' && \
                  /tmp/irods_setup.sh" \
       --login postgres && \
    yum --assumeyes remove cpp perl perl-JSON && \
    yum clean all && \
    rm --force --recursive /tmp/* /logs/* /var/cache/yum

EXPOSE "$DBMS_PORT"/tcp

VOLUME /data /logs

COPY service.sh /

CMD [ "/service.sh" ]
