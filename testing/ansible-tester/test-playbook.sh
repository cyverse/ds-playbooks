#! /bin/bash
#
# Usage:
#  test-playbook PLAYBOOK
#
# Parameters:
#  PLAYBOOK  the name of the playbook being tested.
#
# This program executes and ansible playbook on the test environment.


main()
{
  local playbook="$1"

  printf 'Waiting for environment to be ready\n'

  if ansible-playbook --inventory-file=/inventory /wait-for-ready.yml > /dev/null
  then
    printf 'Running playbook\n'
    ansible-playbook --inventory-file=/inventory /playbooks-under-test/"$playbook"
  fi
}


set -e

main "$*"
