FROM ubuntu:22.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN mkdir -m 700 -p /etc/geopm-service; mkdir -m 700 -p /etc/geopm-service/0.DEFAULT_ACCESS; echo "TIME" > /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals; chmod 600 /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
RUN apt-get update
RUN apt install -yq software-properties-common
RUN add-apt-repository ppa:cmcantal/geopm
ADD "https://www.random.org/cgi-bin/randbyte?nbytes=10format=h" skipcache
RUN apt-get update
RUN apt install -yq geopm-service
RUN apt install -yq python3-pip
RUN python3 -m pip install --ignore-installed grpcio==1.47.5
