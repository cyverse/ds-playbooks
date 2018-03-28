#! /bin/bash
#
# Usage:
#  config.sh VERSION
#
# Parameter:
#  VERSION  the CentOS major version to configure, either 6 or 7.
#
# This script configures the common ansible requirements for either CentOS
# version.
#


main()
{
  if [ "$#" -lt 1 ]
  then
    printf 'The CentOS version number is required as the first argument\n' >&2
    return 1
  fi

  local version="$1"

  rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-"$version"
  yum --assumeyes install epel-release
  rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-EPEL-"$version"
  yum --assumeyes install \
      libselinux-python openssh-server python-pip python-requests python-virtualenv sudo
  yum clean all
  rm --force --recursive /var/cache/yum

  ssh-keygen -q -f /etc/ssh/ssh_host_key -N '' -t rsa
  ssh-keygen -q -f /etc/ssh/ssh_host_dsa_key -N '' -t dsa
  ssh-keygen -q -f /etc/ssh/ssh_host_rsa_key -N '' -t rsa

  sed --in-place '/session    required     pam_loginuid.so/d' /etc/pam.d/sshd
  update_sshd_config
  mkdir --parents /root/.ssh
  chmod 700 /root/.ssh

  chpasswd <<< root:
}


update_sshd_config()
{
  cat <<EOF | sed --in-place --file - /etc/ssh/sshd_config
s/PermitRootLogin without-password/PermitRootLogin yes/
s/#PermitEmptyPasswords no/PermitEmptyPasswords yes/
EOF
}


set -e

main "$@"
