FROM test-env-base

RUN yum install --assumeyes epel-release
RUN rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7
RUN yum install --assumeyes iptables openssl python-pip rsyslog
RUN pip --disable-pip-version-check install pyOpenSSL
RUN mkdir /etc/haproxy

COPY haproxy/config/iptables /etc/sysconfig
