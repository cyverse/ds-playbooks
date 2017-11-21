#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  "$baseDir"/ansible-support/build
  "$baseDir"/env/build
  "$baseDir"/ansible-tester/build
}


set -e

main
