#! /bin/bash

main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  docker build --file "$baseDir"/ansible-support/Dockerfile.centos6 --tag ansible-support:centos6 \
               "$baseDir"/ansible-support

  docker build --file "$baseDir"/ansible-support/Dockerfile.centos7 --tag ansible-support:centos7 \
               "$baseDir"/ansible-support

  docker-compose --file "$baseDir"/env/docker-compose.yml --project-name dstesting build

  docker build --tag dstesting_ansible "$baseDir"/ansible
}


set -e

main
