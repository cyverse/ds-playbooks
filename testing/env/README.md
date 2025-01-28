# env

This folder contains the source for a set of Docker images that are intended to create a iRODS grid to be used for testing the ansible playbooks that configure an iRODS grid.

The environment consists of a set of containers. The `amqp` container hosts a RabbitMQ broker that in turn hosts the `irods` exchange, where the Data Store publishes messages to. The `dbms_configured` container hosts the PostgreSQL server that in turn hosts the ICAT DB. The `provider_configured` container hosts a configured iRODS catalog service provider. The `provider_unconfigured` container hosts an unconfigured service provider. The `consumer_configured_centos` container hosts a configured CentOS catalog service consumer acting as a resource server. The `consumer_configured_ubuntu` container hosts a configured Ubuntu catalog service consumer acting as a resource server. Finally, the  `consumer_unconfigured` container hosts an unconfigured service consumer.

The environment is controlled by docker-compose, but there are three programs that simplify the usage of docker-compose. `build` can be used to create all of the images. `clean` can be used to delete all of the images. Finally, `controller` can be used to bring up or tear down the grid.

The docker-compose file uses a set of environment variables. They can be passed to each of the `build`, `clean`, and `controller` programs in a file that exports them. Here's a complete example include file.

```bash
# The docker-compose project name.
export ENV_NAME=dstesting

# The name of the docker network.
export DOMAIN="$ENV_NAME"_default

# The host name of the PostgreSQL server
export DBMS_HOST="$ENV_NAME"_dbms_configured_1."$DOMAIN"

# The end of the port range available for parallel transfer and reconnections.
# The beginning of the range is 20000.
export IRODS_LAST_EPHEMERAL_PORT=20009

# The name of the primary group the irods service account belongs to on the
# catalog service provider.
export IRODS_PROVIDER_SYSTEM_GROUP=irods_provider

# The name of the storage resource hosted on the configured ubuntu resource
# server
export IRODS_RES_CONF_UBUNTU_NAME=ingestRes

# The URI for the schema used to validate the configuration files or 'off'
export IRODS_SCHEMA_VALIDATION=off

# The absolute path to the vault on the resource servers
export IRODS_VAULT=/var/lib/irods/Vault

# The name of the iRODS zone
export IRODS_ZONE_NAME=testing

# The host name of the configured Centos catalog service consumer
export IRODS_CONSUMER_CONF_UBUNTU_HOST="$ENV_NAME"_consumer_configured_centos_1."$DOMAIN"

# The host name of the configured Ubuntu catalog service consumer
export IRODS_CONSUMER_CONF_UBUNTU_HOST="$ENV_NAME"_consumer_configured_ubuntu_1."$DOMAIN"

# The name of the default resource to use
export IRODS_DEFAULT_RESOURCE="$IRODS_RES_CONF_UBUNTU_NAME"

# The host name of the configured catalog service provider
export IRODS_PROVIDER_CONF_HOST="$ENV_NAME"_provider_configured_1."$DOMAIN"

# The host name of the unconfigured service provider
export IRODS_PROVIDER_UNCONF_HOST="$ENV_NAME"_provider_unconfigured_1."$DOMAIN"
```
