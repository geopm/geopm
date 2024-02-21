#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 DOCKER_REPO"
    exit -1
fi

docker_repo=$1
for dist in tumbleweed ubuntu2204 ubuntu2210; do
    for role in server client; do
        docker_file=geopm-${dist}-${role}.Dockerfile
        docker build -f${docker_file} . | tee ${docker_file}.log
        sha=$(tail -n1 ${docker_file}.log | sed 's|Successfully built \(.*\)|\1|')
        if [[ -z ${sha} ]]; then
            echo Error: Failed to build ${dist}-${role} 1>&2
        else
            docker tag $sha cmcantal/geopm-service:${dist}-${role}
            docker push ${docker_repo}:${dist}-${role}
        fi
    done
done
