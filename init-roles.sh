#! /bin/bash

set -e

readonly ExecName=$(readlink --canonicalize "$0")
readonly PlaybooksDir=$(dirname "$ExecName")

for roleDir in $(find "$PlaybooksDir" -type d -name roles)
do
  reqFile="$roleDir"/requirements.yml

  # ansible-galaxy won't update roles installed through dependencies, so delete all roles before
  # updating
  roles=$(find "$roleDir" -maxdepth 1 -mindepth 1 -type d -printf '%f\n' | sort)
  keepers=$(sed --quiet 's/^!\([^\/]*\)\/.*/\1/p' "$roleDir"/.gitignore | sort --unique)

  for role in $(comm -2 -3 <(echo "$roles") <(echo "$keepers"))
  do
    rm --force --recursive "$roleDir"/"$role"
  done

  ansible-galaxy install --force --role-file="$reqFile" --roles-path="$roleDir"
done
