#!/usr/bin/env bash
#
# Usage:
#  config.sh OS VERSION
#
# Parameter:
#  OS       The name of the operating system being configured.
#  VERSION  the major version of the OS to configure.
#
# This script configures the common ansible requirements

set -o errexit -o nounset -o pipefail

main() {
	if (( $# < 1 )); then
		printf 'The OS name is required as the first argument\n' >&2
		return 1
	fi

	if (( $# < 2 )); then
		printf 'The OS version number is required as the second argument\n' >&2
		return 1
	fi

	local os="$1"
	local version="$2"

	# Install required packages
	if [[ "$os" == centos ]]; then
		install_centos_packages
	else
		install_ubuntu_packages "$version"
	fi

	# Remove root's password
	passwd -d root

	# Configure root ssh access without password
	ssh-keygen -q -f /etc/ssh/ssh_host_key -N '' -t rsa

	if ! [[ -e /etc/ssh/ssh_host_dsa_key ]]; then
		ssh-keygen -q -f /etc/ssh/ssh_host_dsa_key -N '' -t dsa
	fi

	if ! [[ -e /etc/ssh/ssh_host_rsa_key ]]; then
		ssh-keygen -q -f /etc/ssh/ssh_host_rsa_key -N '' -t rsa
	fi

	if [[ "$os" == centos ]]; then
		ssh-keygen -q -f /etc/ssh/ssh_host_ecdsa_key -N '' -t ecdsa
		ssh-keygen -q -f /etc/ssh/ssh_host_ed25519_key -N '' -t ed25519
	fi

	update_pam_sshd_config
	update_sshd_config
	mkdir --parents /var/run/sshd
	mkdir --mode 0700 /root/.ssh
}

# Install the required CentOS packages.
#
# Parameters:
#  version  the CentOS major distribution version number
install_centos_packages() {
	update_centos_repo
	rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-"$version"

	yum --assumeyes install epel-release
	rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-7

	yum --assumeyes install \
		ca-certificates \
		dmidecode \
		iproute \
		iptables-services \
		jq \
		libselinux-python \
		openssh-server \
		python3 \
		python3-dns \
		python3-pip \
		python3-requests \
		python3-virtualenv \
		sudo \
		yum-plugin-versionlock

	yum clean all
	rm --force --recursive /var/cache/yum
}

install_ubuntu_packages() {
	local version="$1"

	apt-get update
	apt-get install --yes apt-utils 2> /dev/null

	apt-get install --yes \
		ca-certificates \
		dmidecode \
		iproute2 \
		jq \
		openssh-server \
		python3 \
		python3-apt \
		python3-dns \
		python3-pip \
		python3-requests \
		python3-selinux \
		python3-virtualenv \
		sudo

	if [[ "$version" != '18.04' ]]; then
		apt install --yes python-is-python3
	fi

	apt-get clean autoclean
	rm --force --recursive /var/lib/apt/lists/*
}

update_centos_repo() {
	cat <<'EOF' > /etc/yum.repos.d/CentOS-Base.repo
[base]
name=CentOS-$releasever - Base
baseurl=http://vault.centos.org/7.9.2009/os/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[updates]
name=CentOS-$releasever - Updates
baseurl=http://vault.centos.org/7.9.2009/updates/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[extras]
name=CentOS-$releasever - Extras
baseurl=http://vault.centos.org/7.9.2009/extras/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF
}

update_centos_repo() {
	cat <<'EOF' > /etc/yum.repos.d/CentOS-Base.repo
[base]
name=CentOS-$releasever - Base
baseurl=http://vault.centos.org/7.9.2009/os/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[updates]
name=CentOS-$releasever - Updates
baseurl=http://vault.centos.org/7.9.2009/updates/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7

[extras]
name=CentOS-$releasever - Extras
baseurl=http://vault.centos.org/7.9.2009/extras/$basearch/
gpgcheck=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-7
EOF
}

update_pam_sshd_config() {
	cat <<'EOF' | sed --in-place --file - /etc/pam.d/sshd
/@include common-auth/{
	i auth	sufficient	pam_permit.so
	:a
	n
	ba
}
EOF
}

update_sshd_config() {
	cat <<'EOF' | sed --in-place  --regexp-extended --file - /etc/ssh/sshd_config
s/#?PermitRootLogin .*/PermitRootLogin yes/
s/#?PermitEmptyPasswords no/PermitEmptyPasswords yes/
EOF
}

main "$@"
