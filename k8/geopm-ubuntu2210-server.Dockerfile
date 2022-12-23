FROM ubuntu:22.10 AS env

ENV DEBIAN_FRONTEND=noninteractive
RUN mkdir -m 700 -p /etc/geopm-service; mkdir -m 700 -p /etc/geopm-service/0.DEFAULT_ACCESS; echo "TIME" > /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals; chmod 600 /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
RUN apt update -qq && apt install -yq wget gpg
RUN wget -qO- https://build.opensuse.org/projects/home:cmcantal:cloud/public_key | gpg --dearmor -o /etc/apt/keyrings/obs-home-cmcantal-cloud.gpg
RUN echo "deb [signed-by=/etc/apt/keyrings/obs-home-cmcantal-cloud.gpg] https://download.opensuse.org/repositories/home:/cmcantal:/cloud/xUbuntu_22.10/ ./" >> /etc/apt/sources.list
ADD "https://www.random.org/cgi-bin/randbyte?nbytes=10format=h" skipcache
RUN apt-get update
RUN apt install -yq geopm-service
RUN apt install -yq python3-pip
RUN python3 -m pip install --ignore-installed grpcio==1.47.5
