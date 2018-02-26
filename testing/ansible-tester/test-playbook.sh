#! /bin/bash
#
# Usage:
#  test-playbook INSPECT PLAYBOOK
#
# Parameters:
#  INSPECT   if this is `true`, a shell will be opened that allows access to the
#            volumes in the env containers.
#  PLAYBOOK  the name of the playbook being tested.
#
# This program executes and ansible playbook on the test environment.


main()
{
  local inspect="$1"
  local playbook="$2"

  printf 'Waiting for environment to be ready\n'

  if ansible-playbook --inventory-file=/inventory /wait-for-ready.yml > /dev/null
  then
    printf 'Running playbook\n'
    ansible-playbook --inventory-file=/inventory /playbooks-under-test/"$playbook"
  fi

  if [ "$inspect" = true ]
  then
    printf 'Opening shell for inspection of volumes\n'
    iinit "$IRODS_PASSWORD"
    (cd /volumes && bash)
  fi
}


set -e

main "$@"
