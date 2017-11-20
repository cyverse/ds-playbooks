#! /bin/bash

main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  "$baseDir"/ansible-support/build
  docker-compose --file "$baseDir"/env/docker-compose.yml --project-name dstesting build
  docker build --tag dstesting_ansible "$baseDir"/ansible
}


set -e

main
