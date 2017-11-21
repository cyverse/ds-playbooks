#! /bin/bash


main()
{
  if [ "$#" -lt 1 ]
  then
    printf 'The name of a folder holding the playbook is requred\n'
    return 1
  fi

  local baseDir=$(dirname $(readlink -f "$0"))
  local playbooks="$1"

  if [ "$#" -ge 2 ]
  then
    playbook="$2"
  else
    playbook=main.yml
  fi

  if ! [[ "$playbooks" =~ ^/ ]]
  then
    playbooks="$(pwd)"/"$playbooks"
  fi

  if "$baseDir"/env/controller start
  then
    docker run --rm --tty \
               --network dstesting_default --volume "$playbooks":/playbooks-under-test:ro \
               ansible-tester "$playbook"
  fi

  "$baseDir"/env/controller stop
}


main "$@"
