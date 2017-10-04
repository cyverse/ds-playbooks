#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly RolesDir=$(dirname "$ExecName")/roles

# ansible-galaxy won't update roles installed through dependencies, so delete all roles before
# updating
for role in $(find "$RolesDir" -maxdepth 1 -mindepth 1 -type d)
do
  rm --force --recursive "$role"
done

ansible-galaxy install --force --role-file="$RolesDir"/requirements.yml --roles-path="$RolesDir"

