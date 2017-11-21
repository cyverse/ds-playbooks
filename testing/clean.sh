#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  docker rmi ansible-tester
  "$baseDir"/env/clean
  "$baseDir"/ansible-support/clean
}


main
