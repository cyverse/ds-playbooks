#! /bin/bash
#
# Usage:
#  service (start|stop)
#
# This script starts or stops the RabbitMQ broker inside the container.


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
  /docker-entrypoint.sh rabbitmq-server &

  until exec 3<> /dev/tcp/localhost/15672
  do
    printf 'Waiting for RabbitMQ\n'
    sleep 1
  done 2> /dev/null
}


stop()
{
  rabbitmqctl stop
}


set -e

main "$@"
