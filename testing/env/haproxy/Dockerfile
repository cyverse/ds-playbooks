FROM test-env-base

RUN yum install --assumeyes epel-release && \
    rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7 && \
    yum install --assumeyes iptables openssl python-pip rsyslog && \
    pip --disable-pip-version-check install pyOpenSSL && \
    yum clean all && \
    rm --force --recursive /var/cache/yum
