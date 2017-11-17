#! /bin/bash

main()
{
  local playbook="$*"

  if ! [ -f "$playbook" ]
  then
    printf 'The absolute path to a playbook is required as the first argument\n' >&2
    return 1
  fi

  if docker-compose --file env/docker-compose.yml --project-name dstesting up -d
  then
    docker run --rm --tty \
               --network dstesting_default --volume "$playbook":/playbook-under-test.yml:ro \
               dstesting_ansible
  fi
  
  docker-compose --file env/docker-compose.yml --project-name dstesting down
}


main "$*"
