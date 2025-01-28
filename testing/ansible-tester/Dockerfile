FROM ubuntu:22.04

ARG IRODS_CLERVER_PASSWORD=rods

ARG DEBIAN_FRONTEND=noninteractive

ADD --chmod=444 \
	https://packages.irods.org/irods-signing-key.asc /etc/apt/trusted.gpg.d/irods-signing-key.asc

COPY apt-preferences-irods /etc/apt/preferences.d/irods

RUN \
	--mount=source=requirements.txt,target=/requirements.txt \
	--mount=source=requirements.yml,target=/requirements.yml \
<<EOF
	set -o errexit

### Update system packages
	apt-get update
	apt-get --yes upgrade
	apt-get install --yes \
		dmidecode gpg-agent lsb-release python-is-python3 python3 python3-pip openssh-client rpm wget
	python3 -m pip install -r /requirements.txt
	ansible-galaxy collection install --requirements-file=/requirements.yml

### Install inspection support
	apt-get install --yes ca-certificates
	printf 'deb [arch=amd64] https://packages.irods.org/apt/ %s main\n' \
			"$(lsb_release --codename --short)" \
		> /etc/apt/sources.list.d/renci-irods.list
	apt-get update
	apt-get install --yes irods-icommands postgresql

# XXX - Due to a bug in iCommands 4.3.1, irods_environment.json needs to exist prior to `iinit`
#       execution. See https://github.com/irods/irods_client_icommands/issues/493
	mkdir /root/.irods
	echo '{}' > /root/.irods/irods_environment.json
# XXX - ^^^

	apt-get clean autoclean
	rm --force --recursive /var/lib/apt/lists/*
EOF

COPY --chmod=400 ssh-config /root/.ssh/config
COPY ansible.cfg /root/.ansible.cfg
COPY inventory /inventory
COPY wait-for-ready.yml /
COPY --chmod=100 test-playbook.sh /test-playbook

VOLUME /playbooks-under-test

ENV IRODS_HOST=localhost
ENV IRODS_PASSWORD=$IRODS_CLERVER_PASSWORD
ENV IRODS_PORT=1247
ENV IRODS_USER_NAME=rods
ENV IRODS_ZONE_NAME=tempZone
ENV PGDATABASE=ICAT
ENV PGHOST=localhost
ENV PGPASSWORD=testpassword
ENV PGUSER=irods

ENTRYPOINT [ "/test-playbook" ]
