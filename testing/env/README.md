# env

This folder contains the source for a set of Docker images that are intended to
create a iRODS grid to be used for testing the ansible playbooks that configure
an iRODS grid.

The environment consists of size containers. The `amqp` container hosts the
RabbitMQ broker that in turn hosts the `irods` exchange, where the Data Store
publishes messages to. The `dbms` container hosts the PostgreSQL server that in
turn hosts the ICAT DB. The `load_balancer` container hosts the HAProxy for the
IES. The `ies_centos6` container hosts the IES server running CentOS 6. The
`ies_centos7` containers hosts the IES server running CentOS 7. The `rs_centos6`
container hosts the resource server running on CentOS 6. Finally, the
`rs_centos7` container hosts the resource server running on CentOS 7.

The environment is controlled by docker-compose, but there are three programs
that simplify the usage of docker-compose. `build` can be used to create all of
the images. `clean` can be used to delete all of the images. Finally,
`controller` can be used to bring up or tear down the grid.

The docker-compose file uses a set of environment variables. They can be passed
to each of the `build`, `clean`, and `controller` programs in a file that
exports them.  Here's a complete example include file.

```bash
# The docker-compose project name.
export ENV_NAME=dstesting

# The name of the docker network.
export DOMAIN="$ENV_NAME"_default

# The host name of the PostgreSQL server
export DBMS_HOST="$ENV_NAME"_dbms_1."$DOMAIN"

# The name of the primary group the irods service account belongs to on IES.
export IRODS_IES_SYSTEM_GROUP=irods_ies

# The end of the port range available for parallel transfer and reconnections.
# The beginning of the range is 20000.
export IRODS_LAST_EPHEMERAL_PORT=20009

# The name of the storage resource hosted on the CentOS 6 resource server
export IRODS_RS6_NAME=rs_centos6

# The name of the storage resource hosted on the CentOS 7 resource server
export IRODS_RS7_NAME=rs_centos7

# The URI for the schema used to validate the configuration files or 'off'
export IRODS_SCHEMA_VALIDATION=off

# The absolute path to the vault on the resource servers
export IRODS_VAULT=/var/lib/irods/Vault

# The name of the iRODS zone
export IRODS_ZONE_NAME=testing

# The name of the default resource to use
export IRODS_DEFAULT_RESOURCE="$IRODS_RS6_NAME"

# The host name of the CentOS 6 IES
export IRODS_IES6_HOST="$ENV_NAME"_ies_centos6_1."$DOMAIN"

# The host name of the CentOS 7 IES
export IRODS_IES7_HOST="$ENV_NAME"_ies_centos7_1."$DOMAIN"

# The host name of the CentOS 6 resource server
export IRODS_RS6_HOST="$ENV_NAME"_"$IRODS_RS6_NAME"_1."$DOMAIN"

# The host name of the CentOS 7 resource server
export IRODS_RS7_HOST="$ENV_NAME"_"$IRODS_RS7_NAME"_1."$DOMAIN"
```
