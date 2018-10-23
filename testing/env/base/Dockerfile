FROM centos:7

COPY base/config.sh /tmp/config

RUN /tmp/config centos 7 && \
    rm --force /tmp/config

COPY base/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
