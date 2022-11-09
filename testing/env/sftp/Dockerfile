FROM test-env-base:ubuntu2204

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get install --yes --quiet=2 locales
RUN locale-gen en_US.UTF-8

ARG LC_COLLATE=C

COPY sftp/service.sh /

CMD [ "/service.sh" ]