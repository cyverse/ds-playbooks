#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  "$baseDir"/ansible-tester/clean
  "$baseDir"/env/clean
  "$baseDir"/ansible-support/clean
}


main
