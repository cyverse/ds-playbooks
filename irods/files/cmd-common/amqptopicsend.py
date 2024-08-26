#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""This publishes messages to a give RabbitMQ exchange."""

import os
import sys
from sys import stderr
from typing import List

import pika

def _publish(uri: str, exchange: str, routing_key: str, body:str) -> None:
    conn_params = pika.URLParameters(uri)
    conn_params.socket_timeout = 10
    with pika.BlockingConnection(conn_params) as conn:
        conn.channel().basic_publish(
            exchange=exchange,
            routing_key=routing_key,
            body=body,
            properties=pika.BasicProperties(delivery_mode=pika.DeliveryMode.Persistent))


def _main(argv: List[str]) -> int:
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
    except BaseException as e:  # pylint: disable=broad-exception-caught
        ex_type = type(e)
        stderr.write(f"Failed to publish message: {e} ({ex_type})\n")
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(_main(sys.argv))
