#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  "$baseDir"/ansible-support/build
  "$baseDir"/env/build
  docker build --tag ansible-tester "$baseDir"/ansible-tester
}


set -e

main
