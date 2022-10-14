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
    eval $(cat /etc/sftpgo/sftpgo.conf | sed 's/^/export /')
    sudo -E -u sftpgo sh -c "cd /var/lib/sftpgo && sftpgo serve --config-file /etc/sftpgo/sftpgo.json &"
  fi
}


stop()
{
  if id --user sftpgo &> /dev/null
  then
    # sending SIGINT (Ctrl+C)
    sudo -u sftpgo pkill -SIGINT sftpgo
  fi
}


main "$@"
