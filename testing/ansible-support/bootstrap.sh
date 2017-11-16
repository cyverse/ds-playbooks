#! /bin/bash

start()
{
  if [ -n "$cmd" ]
  then
    eval "$cmd" start
  fi

  service sshd start
}


stop()
{
  service sshd stop

  if [ -n "$cmd" ]
  then
    eval "$cmd" stop
  fi
}


main()
{
  local cmd="$*"

  start "$cmd"
  trap "kill \${!}; stop '$cmd'; exit \$?" SIGTERM

  while true
  do
    tail --follow /dev/null &
    wait ${!}
  done
}


set -e

main "$*"
