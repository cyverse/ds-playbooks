FROM test-env-base:ubuntu2204

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update
RUN apt-get install --yes openssl rsyslog ssl-cert ufw
RUN pip install pyOpenSSL

RUN make-ssl-cert generate-default-snakeoil
RUN mkdir --parents /etc/haproxy/certs
RUN cp /etc/ssl/private/ssl-cert-snakeoil.key /etc/haproxy/certs/dumb.pem
RUN cat /etc/ssl/certs/ssl-cert-snakeoil.pem >> /etc/haproxy/certs/dumb.pem
