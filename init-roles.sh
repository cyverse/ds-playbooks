#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly PlaybooksDir=$(dirname "$ExecName")

for roleDir in $(find "$PlaybooksDir" -type d -name roles)
do
  reqFile="$roleDir"/requirements.yml

  # ansible-galaxy won't update roles installed through dependencies, so delete all roles before
  # updating
  for role in $(sed --quiet 's/^\(-\| \) name: //p' "$reqFile")
  do
    rm --force --recursive "$roleDir"/"$role"
  done

  ansible-galaxy install --force --role-file="$reqFile" --roles-path="$roleDir"
done
