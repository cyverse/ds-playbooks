#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""This sends an email message."""

from email.message import EmailMessage
from smtplib import SMTP
import sys
from sys import stderr
from typing import List


def _send_mail(from_addr: str, to_addr: str, subject: str, body: str) -> None:
    msg = EmailMessage()
    msg['From'] = from_addr
    msg['To'] = to_addr
    msg['Subject'] = subject
    msg.set_content(body)
    with SMTP("localhost") as smtp:
        smtp.send_message(msg)


def _main(argv: List[str]) -> int:
    try:
        from_addr = argv[1]
        to_addr = argv[2]
        subject = argv[3]
        body = argv[4]
    except IndexError:
        stderr.write(
            "The email source address, destination address, subject, and message body are required "
            "as the first four parameters, respectively\n")
        return 1
    try:
        _send_mail(
            from_addr=from_addr,
            to_addr=to_addr,
            subject=subject,
            body=body)
    except BaseException as exn:  # pylint: disable=broad-exception-caught
        stderr.write(f"Failed to send the following message to {to_addr}: {body} ({exn})\n")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(_main(sys.argv))
