#! /bin/bash

main()
{
  local baseDir=$(dirname $(readlink -f "$0"))
  local playbook="$*"

  if ! [ -f "$playbook" ]
  then
    printf 'The absolute path to a playbook is required as the first argument\n' >&2
    return 1
  fi

  if ! [[ "$playbook" =~ ^/ ]]
  then
    playbook="$(pwd)"/"$playbook"
  fi
  
  if docker-compose --file "$baseDir"/env/docker-compose.yml --project-name dstesting up -d
  then
    docker run --rm --tty \
               --network dstesting_default --volume "$playbook":/playbook-under-test.yml:ro \
               dstesting_ansible
  fi

  docker-compose --file "$baseDir"/env/docker-compose.yml --project-name dstesting down
}


main "$*"
