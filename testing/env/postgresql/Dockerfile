FROM postgres:9.3

COPY base/config.sh /tmp/base-config

ARG DEBIAN_FRONTEND=noninteractive

RUN /tmp/base-config debian jesse

# Since the provided value of PGDATA is mounted as a volume and given 777
# permissions, pg_ctl cannot run during build time. The data directory location
# needed to be changed.
ENV PGDATA /var/lib/postgresql/alt_data

RUN mkdir "$PGDATA"
RUN chown postgres:postgres "$PGDATA"
RUN gosu postgres initdb --auth ident --lc-collate C --locale en_US.UTF-8 --pgdata "$PGDATA"

COPY postgresql/scripts/*.sql* /tmp/
COPY postgresql/scripts/config.sh /tmp/config
COPY postgresql/config/* "$PGDATA"/

RUN chown postgres:postgres "$PGDATA"/*

ARG DB_NAME=ICAT
ARG DB_PASSWORD=testpassword
ARG DB_USER=irods
ARG DBMS_PORT=5432
ARG IRODS_RESOURCES=demoResc:rs:/var/lib/irods/Vault
ARG IRODS_ZONE_NAME=tempZone
ARG IRODS_ZONE_PASSWORD=rods
ARG IRODS_ZONE_USER=rods

RUN gosu postgres /tmp/config

COPY base/entrypoint.sh /entrypoint
COPY postgresql/service.sh /

ENTRYPOINT [ "/entrypoint" ]
CMD [ "/service.sh" ]
