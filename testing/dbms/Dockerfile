FROM centos:6
MAINTAINER Tony Edgin @ CyVerse

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_PORT=5432
ARG DBMS_TYPE=postgres
ARG IRODS_IES=ies
ARG IRODS_DEFAULT_RESOURCE=demoResc
ARG IRODS_DEFAULT_VAULT=/var/lib/irods/Vault
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_USER=rods

ENV PGDATA /var/lib/pgsql/9.3

COPY bootstrap.sh /

RUN rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-6 && \
    yum --assumeyes install \
      https://download.postgresql.org/pub/repos/yum/9.3/redhat/rhel-6-x86_64/pgdg-centos93-9.3-3.noarch.rpm && \
    yum --assumeyes install postgresql93-server && \
    yum clean all && \
    mkdir /data /logs && \
    chmod a+rwx /bootstrap.sh /data /logs && \
    chown postgres:postgres /bootstrap.sh /data /logs && \
    sed --in-place "s|^PGDATA=.*|PGDATA='$PGDATA'|" /var/lib/pgsql/.bash_profile && \
    su --command "/usr/pgsql-9.3/bin/initdb --auth ident \
                                            --lc-collate C \
                                            --locale en_US.UTF-8 \
                                            --pgdata /data" \
       --login postgres && \
    mv /data/*.conf "$PGDATA"/

COPY scripts/* /tmp/
COPY config/*.conf "$PGDATA"/

RUN chown postgres:postgres "$PGDATA"/* && \
    yum --assumeyes install cpp gcc-c++ openssl-devel perl perl-JSON && \
    su --command "export DB_NAME='$DB_NAME' && \
                  export DB_PASSWORD='$DB_PASSWORD' && \
                  export DB_USER='$DB_USER' && \
                  export DBMS_PORT='$DBMS_PORT' && \
                  export DBMS_TYPE='$DBMS_TYPE' && \
                  export IRODS_DEFAULT_RESOURCE='$IRODS_DEFAULT_RESOURCE' && \
                  export IRODS_DEFAULT_VAULT='$IRODS_DEFAULT_VAULT' && \
                  export IRODS_IES='$IRODS_IES' && \
                  export IRODS_ZONE_NAME='$IRODS_ZONE_NAME' && \
                  export IRODS_ZONE_PASSWORD='$IRODS_ZONE_PASSWORD' && \
                  export IRODS_ZONE_USER='$IRODS_ZONE_USER' && \
                  /tmp/irods_setup.sh" \
       --login postgres && \
    yum --assumeyes remove cpp gcc-c++ openssl-devel perl perl-JSON && \
    yum clean all && \
    rm --force --recursive /tmp/* /logs/*

EXPOSE "$DBMS_PORT"/tcp

VOLUME /data /logs

USER postgres
WORKDIR /var/lib/pgsql

ENTRYPOINT [ "/bootstrap.sh" ]
