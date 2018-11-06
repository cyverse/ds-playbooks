FROM centos:7
MAINTAINER Tony Edgin @ CyVerse

COPY base/config.sh /tmp/base-config

RUN /tmp/base-config centos 7 && \
    yum install --assumeyes epel-release && \
    rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 && \
    yum install --assumeyes iptables openssl python-pip rsyslog && \
    pip --disable-pip-version-check install pyOpenSSL && \
    mkdir --parents /etc/haproxy && \
    yum clean all && \
    rm --recursive /tmp/* /var/cache/yum

COPY base/entrypoint.sh /entrypoint

ENTRYPOINT [ "/entrypoint" ]
