#!/bin/bash
#
# Usage:
#  test-playbook INSPECT PRETTY PLAYBOOK
#
# Parameters:
#  INSPECT   if this is `true`, a shell will be opened that allows access to the
#            volumes in the env containers.
#  PLAYBOOK  the name of the playbook being tested.
#  PRETTY    if this is `true`, more info is dumped and newlines in ouput are
#            expanded.
#  HOSTS     the inventory hosts to test against
#
# This program executes and ansible playbook on the test environment.


main()
{
  local inspect="$1"
  local pretty="$2"
  local playbook="$3"
  local hosts="$4"

  if [ "$pretty" = true ]
  then
    export ANSIBLE_STDOUT_CALLBACK=minimal
  fi

  do_test "$playbook" "$hosts"

  if [ "$inspect" = true ]
  then
    printf 'Opening shell for inspection of volumes\n'
    iinit "$IRODS_PASSWORD"
    bash
  fi

  return 0
}


do_test()
{
  local playbook="$1"
  local hosts="$2"

  local inventory=/inventory/"$hosts"
  local playbookPath=/playbooks-under-test/"$playbook"
  local testPath=/playbooks-under-test/tests/"$playbook"

  printf 'Waiting for environment to be ready\n'
  if ! ansible-playbook --inventory-file "$inventory" /wait-for-ready.yml > /dev/null
  then
    return 1
  fi

  printf 'Checking playbook syntax\n'
  if ! ansible-playbook --syntax-check --inventory-file "$inventory" "$playbookPath"
  then
    return 1
  fi

  printf 'Running playbook\n'
  if ! ansible-playbook --inventory-file "$inventory" --skip-tags no_testing "$playbookPath"
  then
    return 1
  fi

  local libPathOption=""
  # add the option for module-path only if a library directory exists
  if [ -d /playbooks-under-test/library ]
  then
    libPathOption="--module-path /playbooks-under-test/library"
  fi

  if [ -e "$testPath" ]
  then
    printf 'Checking configuration\n'
    if ! ansible-playbook --inventory-file "$inventory" "$testPath" ${libPathOption}
    then
      return 1
    fi
  fi

  printf 'Checking idempotency\n'

  local idempotencyRes
  idempotencyRes=$(ansible-playbook --inventory-file "$inventory" \
                                    --skip-tags 'no_testing, non_idempotent' \
                                    "$playbookPath" \
                     2>&1)

  if grep --quiet --regexp '^\(changed\|failed\):' <<< "$idempotencyRes"
  then
    printf '%s\n' "$idempotencyRes"
    return 1
  fi
}


set -u -o pipefail
main "$@"
