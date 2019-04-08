FROM rabbitmq:3.5.1-management

ARG DEBIAN_FRONTEND=noninteractive

ADD http://archive.debian.org/debian-archive/debian-security/pool/updates/main/c/curl/libcurl3-gnutls_7.26.0-1+wheezy25+deb7u1_amd64.deb \
    http://archive.debian.org/debian-archive/debian-security/pool/updates/main/a/apt/apt-transport-https_0.9.7.9+deb7u6_amd64.deb \
    /

RUN dpkg --install \
      /libcurl3-gnutls_7.26.0-1+wheezy25+deb7u1_amd64.deb \
      /apt-transport-https_0.9.7.9+deb7u6_amd64.deb

COPY rabbitmq/config/sources.list /etc/apt
COPY rabbitmq/config/rabbitmq.list /etc/apt/sources.list.d

RUN apt-key adv --keyserver hkps.pool.sks-keyservers.net --recv-keys 0x6B73A36E6026DFCA

COPY base/config.sh /tmp/base-config

RUN /tmp/base-config debian wheezy

COPY base/entrypoint.sh /entrypoint
COPY rabbitmq/service.sh /

ENTRYPOINT [ "/entrypoint" ]
CMD [ "/service.sh" ]
