FROM test-env-base:centos7

RUN yum install --assumeyes openssl
RUN /etc/pki/tls/certs/make-dummy-cert /etc/pki/tls/certs/testing.crt
