#! /bin/bash
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
	if [[ "$os" = centos ]]; then
		install_centos_packages "$version"
	elif [[ "$os" == debian ]]; then
		install_debian_packages "$version"
	else
		install_ubuntu_packages "$version"
	fi

	# Remove root's password
	passwd -d root

	# Configure passwordless root ssh access
	ssh-keygen -q -f /etc/ssh/ssh_host_key -N '' -t rsa

	if ! [[ -e /etc/ssh/ssh_host_dsa_key ]]; then
		ssh-keygen -q -f /etc/ssh/ssh_host_dsa_key -N '' -t dsa
	fi

	if ! [[ -e /etc/ssh/ssh_host_rsa_key ]]; then
		ssh-keygen -q -f /etc/ssh/ssh_host_rsa_key -N '' -t rsa
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
	local version="$1"

	rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-"$version"

	yum --assumeyes install yum-plugin-versionlock

	yum --assumeyes install epel-release
	rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-"$version"

	yum --assumeyes install \
		ca-certificates iproute iptables-services jq libselinux-python openssh-server python2-pip sudo
}


install_debian_packages() {
	local version="$1"

	apt-get update -qq
	apt-get install -qq -y apt-utils 2> /dev/null

	apt-get install -qq -y \
			ca-certificates \
			iproute \
			iptables \
			jq \
			openssh-server \
			python-pip \
			python-selinux \
			python-virtualenv \
			sudo \
		2> /dev/null
}


install_ubuntu_packages() {
	local version="$1"

	apt-get update --quiet=2
	apt-get install --yes --quiet=2 apt-utils 2> /dev/null

	apt-get install --yes --quiet=2 \
		ca-certificates \
		dmidecode \
		iproute2 \
		jq \
		openssh-server \
		python3-apt \
		python3-dns \
		python3-pip \
		python3-requests \
		python3-selinux \
		python3-virtualenv \
		sudo
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
