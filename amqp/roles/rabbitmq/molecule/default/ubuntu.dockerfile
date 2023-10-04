FROM ubuntu:22.04

RUN apt update && apt install --yes systemd
CMD [ "/lib/systemd/systemd" ]
