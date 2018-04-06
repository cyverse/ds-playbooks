FROM centos:7
MAINTAINER Tony Edgin @ CyVerse

COPY ansible-support/config.sh /tmp/ansible-support

RUN /tmp/ansible-support centos 7 && \
    yum install --assumeyes iptables rsyslog && \
    yum clean all && \
    rm --recursive /tmp/* /var/cache/yum && \
    touch /etc/sysconfig/iptables

COPY ansible-support/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
