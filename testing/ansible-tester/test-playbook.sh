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

  do_test "$playbook"

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

  local playbookPath=/playbooks-under-test/"$playbook"
  local testPath=/playbooks-under-test/tests/"$playbook"

  printf 'Waiting for environment to be ready\n'
  if ! run_playbook /wait-for-ready.yml > /dev/null
  then
    return 1
  fi

  printf 'Checking playbook syntax\n'
  if ! run_playbook --syntax-check "$playbookPath"
  then
    return 1
  fi

  printf 'Running playbook\n'
  if ! run_playbook --skip-tags no_testing "$playbookPath"
  then
    return 1
  fi

  if [ -e "$testPath" ]
  then
    printf 'Checking configuration\n'
    if ! run_playbook "$testPath"
    then
      return 1
    fi
  fi

  printf 'Checking idempotency\n'
  run_playbook --skip-tags 'no_testing, non_idempotent' "$playbookPath" 2>&1 \
    | sed --quiet '/^PLAY RECAP/ { s///; :a; n; /changed=\([^0]\|0.*failed=[^0]\)/p; ba; }' \
    | if read
      then
        printf 'not idempotent\n'
        return 1
      fi
}


run_playbook()
{
  ansible-playbook --inventory-file=/inventory "$@"
}


main "$@"
