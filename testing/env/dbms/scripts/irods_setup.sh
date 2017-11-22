#!/bin/bash


# escapes / and \ for sed script
escape()
{
  local var="$*"

  # Escape \ first to avoid escaping the escape character, i.e. avoid / -> \/ -> \\/
  var="${var//\\/\\\\}"

  printf '%s' "${var//\//\\/}"
}


irods_setup_dbms()
{
  local dbmsPort="$1"
  local dbName="$2"
  local dbAdmName="$3"
  local dbAdmPasswd="$4"

  # Create the tables.
  # The iCAT SQL files issue a number of instructions to create tables and
  # initialize state.

  print_msg  $'    \n' 'Creating iCAT tables...'

  local sqlfiles=(icat-sys-tables.sql icat-sys-inserts.sql icat-setup-values.sql)

  print_msg  $'    \n' '    Inserting iCAT tables...'

  local serverSqlDir=/tmp
  local psql="$(/tmp/find-bin-postgres)"/psql

  for sqlfile in ${sqlfiles[@]}
  do
    local sqlPath="$serverSqlDir"/"$sqlfile"

    local output=$(PGPASSWORD="$dbAdmPasswd" \
                   "$psql" --port "$dbmsPort" --user "$dbAdmName" "$dbName" < "$sqlPath" 2>&1)

    local status="$?"

    if [ "$status" -ne 0 ]
    then
      # Stop if any of the SQL scripts fails.
      printf '\nInstall problem:\n' >&2
      printf '    Could not create the iCAT tables.\n' >&2
      print_msg $'        \n' "$output" >&2
      printf '\nAbort.\n' >&2
      return 1
    fi

    if [[ "${output,,}" =~ error ]]
    then
      printf '\nInstall problem:\n' >&2
      printf '    Creation of the iCAT tables failed.\n' >&2
      print_msg $'        \n' "$output" >&2
      printf '\nAbort.\n' >&2
      return 1
    fi
  done
}


print_msg()
{
  local message="$*"

	if [ -z "$message" ]
  then
    return;
  fi

	# If the first message line is just white space, then presume it is an indent
  # for the rest of the message lines. But if there's nothing more in the
  # message array, then it isn't an indent.

  IFS=$'\n' msgLines=($message)

  local indent=

	if [[ "${msgLines[0]}" =~ ^[[:space:]]+$ ]]
	then
		indent="${msgLines[0]}"
    msgLines=(${msgLines[@]:1})
	fi

	# Print the message line-by-line.
  for entry in "${msgLines[@]}"
  do
		printf '%s%s\n' "$indent" "$entry"
	done <<< "$message"
}


# prepare SQL from template
readonly ICATSetupValues=/tmp/icat-setup-values.sql

printf 'Preparing %s...\n' "$ICATSetupValues"

cat <<EOF | sed --file - "$ICATSetupValues".template > "$ICATSetupValues"
s/ZONE_NAME_TEMPLATE/$(escape $IRODS_ZONE_NAME)/g
s/ADMIN_NAME_TEMPLATE/$(escape $IRODS_ZONE_USER)/g
s/ADMIN_PASSWORD_TEMPLATE/$(escape $IRODS_ZONE_PASSWORD)/g
EOF

id=9101

while IFS=: read -r name server vault
do
  cat <<EOF
INSERT INTO R_RESC_MAIN (
  resc_id, resc_name, zone_name,          resc_type_name,   resc_class_name, resc_net,
  resc_def_path, free_space, free_space_ts, resc_info, r_comment, resc_status, create_ts,
  modify_ts)
VALUES (
  $id,     '$name',   '$IRODS_ZONE_NAME', 'unixfilesystem', 'cache',         '$server',
  '$vault',      '',         '',            '',        '',        '',          '1170000000',
  '1170000000');
EOF

((id++))
done < <(tr ' ' '\n' <<< "$IRODS_RESOURCES") >> "$ICATSetupValues"

/usr/pgsql-9.3/bin/pg_ctl -w start
psql --command "CREATE DATABASE \"$DB_NAME"\"
psql --command "CREATE USER $DB_USER WITH PASSWORD '$DB_PASSWORD'"
psql --command "GRANT ALL PRIVILEGES ON DATABASE \"$DB_NAME\" TO $DB_USER"
irods_setup_dbms "$DBMS_PORT" "$DB_NAME" "$DB_USER" "$DB_PASSWORD"
/usr/pgsql-9.3/bin/pg_ctl -w stop
