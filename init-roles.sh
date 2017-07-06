#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly PlaybookDir=$(dirname "$ExecName")/playbooks

for roleDir in $(find "$PlaybookDir" -type d -name roles)
do
  # ansible-galaxy won't update roles installed through dependencies, so delete all roles before
  # updating
  for role in $(find "$roleDir" -maxdepth 1 -mindepth 1 -type d)
  do
    rm --force --recursive "$role"
  done
  ansible-galaxy install --force --role-file="$roleDir"/requirements.yml --roles-path="$roleDir"
done
