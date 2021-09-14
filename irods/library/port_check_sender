#!/usr/bin/python
#
# An asible module for connecting to a set of ports on a given host. It is meant
# to work in tandem with port_check_receiver to verify networks routes are open.
#
# Module Name:
#  port_check_sender
#
# Optional Variables:
#  destination  the host to send the msg to, default 'localhost'
#  tcp_ports    a list of TCP ports to send msg over, default is []
#  udp_ports    a list of UDP ports to send msg over, devault is []
#  timeout      the amount of time in seconds to wait for a response from
#               port_check_receiver, default is 4.
#  msg          the message to send, default is 'ping'
#
# This module is meant to work with the port_check_receiver module to check if a
# set of TCP and/or UDP ports on the destination hpst running
# port_check_receiver are reachable from the host running port_check_sender. It
# attempts to send msg to the destination host on each port. It makes note of
# each port that isn't reachable. If any port isn't reachable, it fails,
# reporting all unreachable ports in the failure message


import socket

from ansible.module_utils.basic import AnsibleModule


class Checker:

  def check(self, destination, ports, timeout, msg):
    blocked_ports = []
    for port in ports:
      try:
        sock = self.mk_socket()
        sock.settimeout(timeout)
        self.check_on(sock, (destination, port), msg)
      except Exception as e:
        blocked_ports.append("%d/%s (%s)" % (port, self.protocol(), str(e)))
      try:
        self.close_socket(sock)
      except:
        pass
    return blocked_ports


class TCPChecker(Checker):

  def check_on(self, sock, address, msg):
    sock.connect(address)
    sock.sendall(msg.encode('utf-8'))
    sock.recv(128)

  def close_socket(self, sock):
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

  def mk_socket(self):
    return socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  def protocol(self):
    return "tcp"


class UDPChecker(Checker):

  def check_on(self, sock, address, msg):
    sock.sendto(msg.encode('utf-8'), address)
    sock.recvfrom(128)

  def close_socket(self, sock):
    sock.close()

  def mk_socket(self):
    return socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

  def protocol(self):
    return "udp"


def check(params):
  destination = params['destination']
  tcp_ports = [ int(i) for i in params['tcp_ports'] ]
  udp_ports = [ int(i) for i in params['udp_ports'] ]
  timeout = int(params['timeout'])
  msg = params['msg']
  blocked_tcp_ports = TCPChecker().check(destination, tcp_ports, timeout, msg)
  blocked_udp_ports = UDPChecker().check(destination, udp_ports, timeout, msg)
  return blocked_tcp_ports + blocked_udp_ports


def main():
  module = AnsibleModule(
    argument_spec = dict(
      destination = dict(type='str', default="localhost"),
      tcp_ports = dict(type='list', default=[]),
      udp_ports = dict(type='list', default=[]),
      timeout = dict(type='int', default=4),
      msg = dict(type='str', default="ping")))

  blocked_ports = check(module.params)

  if blocked_ports:
    msg = "blocked ports %s" % blocked_ports
    module.fail_json(msg=msg)
  else:
    module.exit_json()


if __name__ == '__main__':
  main()
