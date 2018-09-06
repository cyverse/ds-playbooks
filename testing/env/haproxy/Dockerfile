FROM centos:7
MAINTAINER Tony Edgin @ CyVerse

COPY base/config.sh /tmp/base-config

RUN /tmp/base-config centos 7 && \
    yum install --assumeyes iptables rsyslog && \
    pip install --upgrade pip && \
    pip install pyOpenSSL && \
    yum clean all && \
    rm --recursive /tmp/* /var/cache/yum

COPY base/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
