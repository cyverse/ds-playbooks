#! /bin/bash

main()
{
  docker build --file ansible-support/Dockerfile.centos6 --tag ansible-support:centos6 \
               ansible-support

  docker build --file ansible-support/Dockerfile.centos7 --tag ansible-support:centos7 \
               ansible-support

  docker-compose --file env/docker-compose.yml --project-name dstesting build

  docker build --tag dstesting_ansible ansible
}


set -e

main
