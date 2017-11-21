#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  "$baseDir"/ansible-support/build
  docker-compose --file "$baseDir"/env/docker-compose.yml --project-name dstesting build
  docker build --tag ansible-tester "$baseDir"/ansible-tester
}


set -e

main
