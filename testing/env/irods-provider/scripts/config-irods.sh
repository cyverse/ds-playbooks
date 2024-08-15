#!/usr/bin/env bash
#
# This script configures a catalog provider.
#
# It requires the following environment variables to be defined.
#
# DB_NAME                     The name of the DB iRODS will use.
# DB_PASSWORD                 The password used to authenticate DB_USER within
#                             PostgreSQL.
# DB_USER                     The DBMS user iRODS will use to connect to DB_NAME
#                             DB.
# DBMS_HOST                   The FQDN or IP address of the PostgreSQL server
# DBMS_PORT                   The TCP port the PostgreSQL will listen on.
# IRODS_CONTROL_PLANE_KEY     The encryption key required for communicating with
#                             the grid control plane.
# IRODS_CONTROL_PLANE_PORT    The port on which the control plane operates.
# IRODS_DEFAULT_RESOURCE      The name of the default resource to use
# IRODS_FIRST_EPHEMERAL_PORT  The beginning of the port range available for
#                             parallel transfer and reconnections.
# IRODS_HOST                  The FQDN or IP address of the server being
#                             configured.
# IRODS_LAST_EPHEMERAL_PORT   The end of the port range available for parallel
#                             transfer and reconnections.
# IRODS_NEGOTIATION_KEY       The shared encryption key used by the zone in
#                             advanced negotiation handshake a the beginning of
#                             a client connection
# IRODS_SCHEMA_VALIDATION     The URI for the schema used to validate the
#                             configuration files or 'off'.
# IRODS_SYSTEM_GROUP          The system group for the iRODS process
# IRODS_SYSTEM_USER           The system user for the iRODS process
# IRODS_ZONE_KEY              The shared secred used for authentication during
#                             server-to-server communication
# IRODS_ZONE_NAME             The name of the iRODS zone.
# IRODS_ZONE_PASSWORD         The password used to authenticate the
#                             IRODS_ZONE_USER user.
# IRODS_ZONE_PORT             The main TCP port used by the zone for
#                             communication.
# IRODS_ZONE_USER             The main rodsadmin user.

set -o errexit -o nounset -o pipefail

main() {
	update_odbc_def
	setup_irods
}

setup_irods() {
	mk_unattended_install > /tmp/resolved_installation.json

	# NOTE: This will fail, because the post-install test fails due to the default storage resource
	# not existing yet.
	if ! \
		python3 /var/lib/irods/scripts/setup_irods.py --verbose \
			--json_configuration_file=/tmp/resolved_installation.json
	then
		echo Ignoring expected failure >&2
	fi

	rm --force /tmp/resolved_installation.json
}

mk_unattended_install() {
	cat <<EOF
{
	"admin_password": "$IRODS_ZONE_PASSWORD",
	"default_resource_name": "$IRODS_DEFAULT_RESOURCE",
	"host_system_information": {
		"service_account_user_name": "$IRODS_SYSTEM_USER",
		"service_account_group_name": "$IRODS_SYSTEM_GROUP"
	},
	"server_config": {
		"advanced_settings": {
			"agent_factory_watcher_sleep_time_in_seconds": 5,
			"default_number_of_transfer_threads": 4,
			"default_temporary_password_lifetime_in_seconds": 120,
			"delay_rule_executors": [],
			"delay_server_sleep_time_in_seconds": 30,
			"dns_cache": {
				"cache_clearer_sleep_time_in_seconds": 600,
				"eviction_age_in_seconds": 3600,
				"shared_memory_size_in_bytes": 5000000
			},
			"host_name_cache": {
				"cache_clearer_sleep_time_in_seconds": 600,
				"eviction_age_in_seconds": 3600,
				"shared_memory_size_in_bytes": 2500000
			},
			"maximum_size_for_single_buffer_in_megabytes": 32,
			"maximum_size_of_delay_queue_in_bytes": 0,
			"maximum_temporary_password_lifetime_in_seconds": 1000,
			"migrate_delay_server_sleep_time_in_seconds": 5,
			"number_of_concurrent_delay_rule_executors": 4,
			"stacktrace_file_processor_sleep_time_in_seconds": 10,
			"transfer_buffer_size_for_parallel_transfer_in_megabytes": 4,
			"transfer_chunk_size_for_parallel_transfer_in_megabytes": 40
		},
		"catalog_provider_hosts": [ "$IRODS_HOST" ],
		"catalog_service_role": "provider",
		"client_api_allowlist_policy": "enforce",
		"controlled_user_connection_list": {
			"control_type": "denylist",
			"users": []
		},
		"default_dir_mode": "0750",
		"default_file_mode": "0600",
		"default_hash_scheme": "SHA256",
		"default_resource_name": "$IRODS_DEFAULT_RESOURCE",
		"environment_variables": {},
		"federation": [],
		"host_access_control": { "access_entries": [] },
		"host_resolution": { "host_entries": [] },
		"log_level": {
			"agent": "info",
			"agent_factory": "info",
			"api": "info",
			"authentication": "info",
			"database": "info",
			"delay_server": "info",
			"legacy": "info",
			"microservice": "info",
			"network": "info",
			"resource": "info",
			"rule_engine": "info",
			"server": "info",
			"sql": "info"
		},
		"match_hash_policy": "compatible",
		"negotiation_key": "$IRODS_NEGOTIATION_KEY",
		"plugin_configuration": {
			"authentication": {},
			"database": {
				"postgres": {
					"db_host": "$DBMS_HOST",
					"db_name": "$DB_NAME",
					"db_odbc_driver": "PostgreSQL",
					"db_password": "$DB_PASSWORD",
					"db_port": $DBMS_PORT,
					"db_username": "$DB_USER"
				}
			},
			"network": {},
			"resource": {},
			"rule_engines": [
				{
					"instance_name": "irods_rule_engine_plugin-irods_rule_language-instance",
					"plugin_name": "irods_rule_engine_plugin-irods_rule_language",
					"plugin_specific_configuration": {
						 "re_data_variable_mapping_set": [ "core" ],
						 "re_function_name_mapping_set": [ "core" ],
						 "re_rulebase_set": [
						 	"pre_config",
							"core"
						 ],
						 "regexes_for_supported_peps": [
							  "ac[^ ]*",
							  "msi[^ ]*",
							  "[^ ]*pep_[^ ]*_(pre|post|except|finally)"
						 ]
					},
					"shared_memory_instance": "irods_rule_language_rule_engine"
				},
				{
					"instance_name": "irods_rule_engine_plugin-cpp_default_policy-instance",
					"plugin_name": "irods_rule_engine_plugin-cpp_default_policy",
					"plugin_specific_configuration": {}
				}
			]
		},
		"rule_engine_namespaces": [ "" ],
		"schema_name": "server_config",
		"schema_validation_base_uri": "$IRODS_SCHEMA_VALIDATION",
		"schema_version": "v4",
		"server_control_plane_encryption_algorithm": "AES-256-CBC",
		"server_control_plane_encryption_num_hash_rounds": 16,
		"server_control_plane_key": "$IRODS_CONTROL_PLANE_KEY",
		"server_control_plane_port": $IRODS_CONTROL_PLANE_PORT,
		"server_control_plane_timeout_milliseconds": 10000,
		"server_port_range_end": $IRODS_LAST_EPHEMERAL_PORT,
		"server_port_range_start": $IRODS_FIRST_EPHEMERAL_PORT,
		"zone_auth_scheme": "native",
		"zone_key": "$IRODS_ZONE_KEY",
		"zone_name": "$IRODS_ZONE_NAME",
		"zone_port": $IRODS_ZONE_PORT,
		"zone_user": "$IRODS_ZONE_USER"
	},
	"service_account_environment": {
		"irods_client_server_negotiation": "request_server_negotiation",
		"irods_client_server_policy": "CS_NEG_REFUSE",
		"irods_connection_pool_refresh_time_in_seconds": 300,
		"irods_cwd": "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER",
		"irods_default_hash_scheme": "SHA256",
		"irods_default_number_of_transfer_threads": 4,
		"irods_default_resource": "$IRODS_DEFAULT_RESOURCE",
		"irods_encryption_algorithm": "AES-256-CBC",
		"irods_encryption_key_size": 32,
		"irods_encryption_num_hash_rounds": 16,
		"irods_encryption_salt_size": 8,
		"irods_home": "/$IRODS_ZONE_NAME/home/$IRODS_ZONE_USER",
		"irods_host": "$IRODS_HOST",
		"irods_match_hash_policy": "compatible",
		"irods_maximum_size_for_single_buffer_in_megabytes": 32,
		"irods_port": $IRODS_ZONE_PORT,
		"irods_server_control_plane_encryption_algorithm": "AES-256-CBC",
		"irods_server_control_plane_encryption_num_hash_rounds": 16,
		"irods_server_control_plane_key": "$IRODS_CONTROL_PLANE_KEY",
		"irods_server_control_plane_port": $IRODS_CONTROL_PLANE_PORT,
		"irods_transfer_buffer_size_for_parallel_transfer_in_megabytes": 4,
		"irods_user_name": "$IRODS_ZONE_USER",
		"irods_zone_name": "$IRODS_ZONE_NAME",
		"schema_name": "service_account_environment",
		"schema_version": "v4"
	}
}
EOF
}

update_odbc_def() {
	odbcinst -i -d -r -v <<EOF
[PostgreSQL]
Description = PostgreSQL 12 ODBC Driver
Driver = /usr/pgsql-12/lib/psqlodbc.so
Setup = /usr/pgsql-12/lib/psqlodbcw.so
EOF
}

main "$@"
