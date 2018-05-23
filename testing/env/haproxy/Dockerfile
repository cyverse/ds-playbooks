FROM centos:7
MAINTAINER Tony Edgin @ CyVerse

COPY base/config.sh /tmp/base-config

RUN /tmp/base-config centos 7 && \
    yum install --assumeyes iptables rsyslog && \
    yum clean all && \
    rm --recursive /tmp/* /var/cache/yum && \
    touch /etc/sysconfig/iptables

COPY base/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
