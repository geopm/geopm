#!/bin/bash
cp -p geopm-server.sh geopm-client.sh /tmp
kubectl create -f manifest.yml
sleep 30
kubectl exec pods/geopm-service-pod -c geopm-server -- /tmp/geopm-server.sh
