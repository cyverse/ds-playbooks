#!/usr/bin/env bash
#
# This script scripts the service.sh script for resource servers
#
# It requires the following environment variables to be defined
#
# IRODS_CATALOG_PROVIDER  the FQDN or IP address of the catalog service provider
# IRODS_SYSTEM_GROUP      the system group for the iRODS process
# IRODS_SYSTEM_USER       the system user for the iRODS process
# IRODS_ZONE_PASSWORD     the password used to authenticate the clever user
# IRODS_ZONE_PORT         the main TCP port used by the zone for communication

set -e


main()
{
  expand_tmpl > /service.sh
  chmod a+rx /service.sh
}


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


expand_tmpl()
{
  cat <<EOF | sed --file - /tmp/service.sh.template
s/\$IRODS_CATALOG_PROVIDER/$(escape "$IRODS_CATALOG_PROVIDER")/g
s/\$IRODS_SYSTEM_GROUP/$(escape "$IRODS_SYSTEM_GROUP")/g
s/\$IRODS_SYSTEM_USER/$(escape "$IRODS_SYSTEM_USER")/g
s/\$IRODS_ZONE_PASSWORD/$(escape "$IRODS_ZONE_PASSWORD")/g
s/\$IRODS_ZONE_PORT/$(escape "$IRODS_ZONE_PORT")/g
EOF
}


main "$@"
