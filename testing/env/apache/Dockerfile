FROM test-env-base:centos7

RUN <<EOF
	set -o errexit
	yum install --assumeyes openssl
	yum clean all
	rm --force --recursive /var/cache/yum
EOF
