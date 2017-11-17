#! /bin/bash

main()
{
  docker rmi \
    dstesting_ansible \
    dstesting_rs_centos6 dstesting_rs_centos7 dstesting_ies dstesting_dbms \
    ansible-support:centos6 ansible-support:centos7
}


main
