#!/bin/bash

# must be run as root

TARGET_NODE=mcfly9

ssh $TARGET_NODE mkdir -p /etc/geopm
scp node_policy.json.example $TARGET_NODE:/etc/geopm/node_policy.json
scp environment-override.json.example $TARGET_NODE:/etc/geopm/environment-override.json
