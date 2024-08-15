FROM test-env-base:ubuntu2204

ARG DEBIAN_FRONTEND=noninteractive
ARG LC_COLLATE=C

RUN <<EOF
	set -o errexit
	apt-get update
	apt-get install --yes locales unattended-upgrades
	locale-gen en_US.UTF-8
	apt-get clean autoclean
	rm --force --recursive /var/lib/apt/list/*
EOF

COPY sftp/service.sh /

CMD [ "/service.sh" ]
