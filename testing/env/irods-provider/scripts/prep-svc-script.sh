#!/usr/bin/env bash
#
# This script creates the service.sh script for an catalog service provider.
#
# It requires the following environment variables to be defined
#
# DBMS_HOST            The FQDN or IP address of the PostgreSQL server
# DBMS_PORT            The TCP port the PostgreSQL will listen on.
# IRODS_SYSTEM_GROUP   The system group for the iRODS process
# IRODS_SYSTEM_USER    The system user for the iRODS process
# IRODS_ZONE_PASSWORD  The password used to authenticate the clever user.


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
s/\$DBMS_HOST/$(escape "$DBMS_HOST")/g
s/\$DBMS_PORT/$(escape "$DBMS_PORT")/g
s/\$IRODS_SYSTEM_GROUP/$(escape "$IRODS_SYSTEM_GROUP")/g
s/\$IRODS_SYSTEM_USER/$(escape "$IRODS_SYSTEM_USER")/g
s/\$IRODS_ZONE_PASSWORD/$(escape "$IRODS_ZONE_PASSWORD")/g
EOF
}


set -e
main
