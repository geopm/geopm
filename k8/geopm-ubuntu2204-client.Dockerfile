FROM ubuntu:22.04 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt install -yq software-properties-common
RUN add-apt-repository ppa:cmcantal/geopm
ADD "https://www.random.org/cgi-bin/randbyte?nbytes=10format=h" skipcache
RUN apt-get update
RUN apt install -yq geopm-runtime geopm-service
