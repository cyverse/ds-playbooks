#! /bin/bash

set -e

yum --assumeyes install epel-release openssh-server sudo
yum clean all
rm --force --recursive /var/cache/yum

ssh-keygen -q -f /etc/ssh/ssh_host_key -N '' -t rsa
ssh-keygen -q -f /etc/ssh/ssh_host_dsa_key -N '' -t dsa
ssh-keygen -q -f /etc/ssh/ssh_host_rsa_key -N '' -t rsa

sed --in-place '/session    required     pam_loginuid.so/d' /etc/pam.d/sshd

cat <<EOF | sed --in-place --file - /etc/ssh/sshd_config
s/PermitRootLogin without-password/PermitRootLogin yes/
s/#PermitEmptyPasswords no/PermitEmptyPasswords yes/
EOF

mkdir --parents /root/.ssh
chmod 700 /root/.ssh

chpasswd <<< root:
