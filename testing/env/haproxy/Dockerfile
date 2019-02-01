FROM centos:7

COPY base/config.sh /tmp/base-config

RUN /tmp/base-config centos 7
RUN rm --force /tmp/base-config
RUN yum install --assumeyes iptables openssl python-pip rsyslog
RUN pip --disable-pip-version-check install pyOpenSSL
RUN mkdir /etc/haproxy

COPY haproxy/config/iptables /etc/sysconfig
COPY base/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
