#! /bin/bash
#
# Usage:
#  service (start|stop)
#
# This script starts or stops the sftpgo server inside the container.

set -o errexit


main()
{
  if [ "$#" -lt 1 ]
  then
    printf 'Requires either "start" or "stop" as its first parameter\n' >&2
    return 1
  fi

  local action="$1"

  case "$action" in
    start)
      start
      ;;
    stop)
      stop
      ;;
    *)
      printf 'Unknown command "%s"\n' "$action" >&2
      return 1
      ;;
  esac
}



start()
{
  if id --user sftpgo &> /dev/null
  then
    sudo -i -u sftpgo sftpgo serve
  fi
}


stop()
{
  if id --user sftpgo &> /dev/null
  then
    # sending SIGINT (Ctrl+C)
    sudo -i -u sftpgo pkill -SIGINT sftpgo
  fi
}


main "$@"
