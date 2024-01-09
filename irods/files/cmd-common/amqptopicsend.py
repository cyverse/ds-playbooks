#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""This publishes messages to a give RabbitMQ exchange."""

import os
import sys
from sys import stderr
from typing import List

import pika

def _publish(uri: str, exchange: str, routing_key: str, body:str) -> None:
    connParams = pika.URLParameters(uri)

    with pika.BlockingConnection(connParams) as conn:
        conn.channel().basic_publish(
            exchange=exchange,
            routing_key=routing_key,
            body=body,
            properties=pika.BasicProperties(delivery_mode=pika.DeliveryMode.Persistent))


def main(argv: List[str]) -> int:
    try:
        exchange = argv[1]
        key = argv[2]
        body = argv[3]
    except IndexError:
        stderr.write(
            "The exchange, routing key, and message body are required as the first three "
            "parameters, respectively\n")
        return 1

    try:
        uri = os.environ['IRODS_AMQP_URI']
        _publish(uri=uri, exchange=exchange, routing_key=key, body=body)
    except BaseException as e:
        stderr.write(f"Failed to publish message: {e}\n")
        return 1

    return 0


if __name__ == '__main__':
# XXX - msiExecCmd core dumps when this returns an error. This is present in iRODS 4.2.8.
#     sys.exit(main(sys.argv))
    main(sys.argv)
    sys.exit(0)
# XXX - ^^^
