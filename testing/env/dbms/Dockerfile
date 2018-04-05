FROM postgres:9.3
MAINTAINER Tony Edgin @ CyVerse

# Since the provided value of PGDATA is mounted as a volume and given 777
# permissions, pg_ctl cannot run during build time. The data directory location
# needed to be changed.
ENV PGDATA /var/lib/postgresql/alt_data

COPY ansible-support/config.sh /tmp/ansible-support

ARG DEBIAN_FRONTEND=noninteractive

RUN /tmp/ansible-support debian jesse && \
    apt-get autoremove --quiet --yes && \
    apt-get clean --quiet && \
    mkdir "$PGDATA" && \
    chown postgres:postgres "$PGDATA" && \
    gosu postgres initdb --auth ident --lc-collate C --locale en_US.UTF-8 --pgdata "$PGDATA"

COPY dbms/scripts/*.sql* /tmp/
COPY dbms/scripts/config.sh /tmp/config
COPY dbms/config/* "$PGDATA"/

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_PORT=5432
ARG IRODS_RESOURCES=demoResc:rs:/var/lib/irods/Vault
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_USER=rods

RUN chown postgres:postgres "$PGDATA"/* && \
    gosu postgres /tmp/config && \
    rm --recursive /tmp/*

COPY ansible-support/entrypoint.sh /entrypoint
COPY dbms/service.sh /

ENTRYPOINT [ "/entrypoint" ]
CMD [ "/service.sh" ]
