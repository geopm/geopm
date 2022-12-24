#!/bin/bash
cp -p geopm-server.sh geopm-client.sh /tmp
kubectl create -f manifest.yml
