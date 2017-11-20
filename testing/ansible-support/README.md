# ansible-support

This folder contains the source for a Docker image that is intended to be used
as a base image for containers that can be configured remotely through ansible
for the purpose of testing ansible playbooks.

The image provides an OpenSSH server as well as pip, sudo, and virtualenv
support. libselinux-python and the python requests module are installed for
convenience. Finally, it provides the EPEL repository.


## Tags

There are versions of this image. One is built on top of CentOS 6, and the other
is built on top of CentOS 7.

- `ansible-support:centos6`
- `ansible-support:centos7`


## Usage

Normally, it is used as a base image for another image. This other image will
provide an application or service that is intended to be configured with
Ansible through externally run ansible playbooks. When a container is started
from this other images, any services in the derived image are started.
Afterwards, an OpenSSH server is started on TCP port 22. This means that when
this port becomes available for connection, all of the provided services are
ready to be used in testing.


### Deriving an Image

The base image provides an entry point script, `/entrypoint`. If this script is
executed with any command line arguments, it assumes that these arguments form
an executable that accepts `start` and `stop` as its first argument. When the
container is started, `/entrypoint` calls any provided executable with `start`
as its argument. When that executable returns, it assumes that any services
started are ready. Next, it starts the OpenSSH server and waits. When it
receives a `SIGTERM` signal, it terminates the OpenSSH server and, if provided,
calls the original executable with `stop` as an argument. When this exits, the
entry point script exits.

If the derived image provides services, its Dockerfile should use the `CMD`
instruction to provide the executable used to start and stop these services.
When this executable is run with `start` as it command line argument, it should
start the required services and not exit before they are ready. When it is run
with `stop` as its argument, it should stop these services and not exit before
they are completely stopped.

Here's an example Dockerfile.

```
FROM ansible-support:centos7

RUN yum --assumeyes install \
      https://download.postgresql.org/pub/repos/yum/9.3/redhat/rhel-6-x86_64/pgdg-centos93-9.3-3.noarch.rpm && \
    yum --assumeyes install postgresql93-server && \
    yum clean all && \
    rm --force --recursive /var/cache/yum && \
    su --command '/usr/pgsql-9.3/bin/initdb --auth ident --lc-collate C --locale en_US.UTF-8' \
       --login postgres

COPY /setup-db.sh /tmp/setup-db

RUN su --command /tmp/setup-db --login postgres && \
    rm --force /tmp/setup-db

EXPOSE 5432

COPY control-script.sh /control-script

CMD [ "/control-script" ]
```

Here's the corresponding `control-script.sh`.

```
#! /bin/bash

readonly Action="$1"

su --command "/usr/pgsql-9.3/bin/pg_ctl -w '$Action'" --login postgres
```

### OpenSSH Configuration

For ease of testing, OpenSSH is configured to allow unauthenticated connections
from `root`. Containers based one of these images should never in production,
nor should they be left running attended on publicly accessible machines. As an
added precaution, TCP port 22 is intentionally not exposed. If this port needs
to be exposed, it should be exposed through the derived image used in testing.


## Acknowledgement

This image is based on the `lemonbar/centos6-ssh` image.
