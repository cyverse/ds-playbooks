#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly PlaybooksDir=$(dirname "$ExecName")

for roleDir in $(find "$PlaybooksDir" -type d -name roles)
do
  # ansible-galaxy won't update roles installed through dependencies, so delete all roles before
  # updating
  for role in $(find "$roleDir" -maxdepth 1 -mindepth 1 -type d)
  do
    rm --force --recursive "$role"
  done

  ansible-galaxy install --force --role-file="$roleDir"/requirements.yml --roles-path="$roleDir"
done
