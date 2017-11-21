#! /bin/bash


main()
{
  local baseDir=$(dirname $(readlink -f "$0"))

  docker rmi \
    dstesting_ansible dstesting_rs_centos6 dstesting_rs_centos7 dstesting_ies dstesting_dbms

  "$baseDir"/ansible-support/clean
}


main
