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
    (cd /volumes && bash)
  fi

  return 0
}


do_test()
{
  local playbook="$1"

  printf 'Waiting for environment to be ready\n'
  if ! playbook /wait-for-ready.yml > /dev/null
  then
    return 1
  fi

  printf 'Checking playbook syntax\n'
  if ! playbook --syntax-check  /playbooks-under-test/"$playbook"
  then
    return 1
  fi

  printf 'Running playbook\n'
  if ! playbook /playbooks-under-test/"$playbook"
  then
    return 1
  fi

  printf 'Checking configuration\n'
  if ! playbook /playbooks-under-test/tests/"$playbook"
  then
    return 1
  fi

  printf 'Checking idempotency\n'
  playbook  /playbooks-under-test/"$playbook" 2>&1 \
    | sed --quiet '/^PLAY RECAP/ { s///; :a; n; /changed=\([^0]\|0.*failed=[^0]\)/p; ba; }' \
    | if read
      then
        printf 'not idempotent\n'
        return 1
      fi
}


playbook()
{
  args="$@"

  ansible-playbook --inventory-file=/inventory "$@"
}


main "$@"
