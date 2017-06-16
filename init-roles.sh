#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly PlaybookDir=$(dirname "$ExecName")/playbooks

for roleDir in $(find "$PlaybookDir" -type d -name roles)
do
  ansible-galaxy install --force --role-file="$roleDir"/requirements.yml --roles-path="$roleDir"
done
