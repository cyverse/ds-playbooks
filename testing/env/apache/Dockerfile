FROM test-env-base

RUN yum install --assumeyes iptables-services openssl
RUN /etc/pki/tls/certs/make-dummy-cert /etc/pki/tls/certs/testing.crt
