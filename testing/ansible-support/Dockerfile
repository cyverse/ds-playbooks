FROM centos:centos6
MAINTAINER Tony Edgin @ CyVerse

# This image is based on the lemonbar/centos6-ssh image, but it allows unauthenticated logins by
# root.

# Configure for ansible
RUN rpm --import file:///etc/pki/rpm-gpg/RPM-GPG-KEY-CentOS-6 && \
    yum --assumeyes install openssh-server && \
    yum clean all && \
    rm --force --recursive /var/cache/yum && \
    ssh-keygen -q -f /etc/ssh/ssh_host_key -N '' -t rsa && \
    ssh-keygen -q -f /etc/ssh/ssh_host_dsa_key -N '' -t dsa && \
    ssh-keygen -q -f /etc/ssh/ssh_host_rsa_key -N '' -t rsa && \
    sed --in-place '/session    required     pam_loginuid.so/d' /etc/pam.d/sshd && \
    sed --in-place \
        's/PermitRootLogin without-password/PermitRootLogin yes/; \
         s/#PermitEmptyPasswords no/PermitEmptyPasswords yes/' \
        /etc/ssh/sshd_config && \
    mkdir --parents /root/.ssh && \
    chmod 700 /root/.ssh && \
    chpasswd <<< root:

COPY bootstrap.sh /

ENTRYPOINT [ "/bootstrap.sh" ]
