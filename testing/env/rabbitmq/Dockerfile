FROM rabbitmq:3.5-management
MAINTAINER Tony Edgin @ CyVerse

ARG DEBIAN_FRONTEND=noninteractive

COPY ansible-support/config.sh /tmp/ansible-support

RUN apt-key adv --keyserver hkps.pool.sks-keyservers.net --recv-keys 0x6B73A36E6026DFCA && \
    /tmp/ansible-support debian jesse && \
    apt-get autoremove --quiet --yes && \
    apt-get clean --quiet && \
    rm --force /tmp/ansible-support

COPY ansible-support/entrypoint.sh /entrypoint
COPY rabbitmq/service.sh /

ENTRYPOINT [ "/entrypoint" ]
CMD [ "/service.sh" ]
