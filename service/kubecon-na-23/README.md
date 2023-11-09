# Getting Started

* Clone the GEOPM cloud branch, and set the env variable that points to the code:
```
git clone https://github.com/geopm/geopm.git -b cloud
cd geopm
export GEOPM_CLOUD_BRANCH=$PWD
```

## Install custom mpi-operator for k8s v.1.28.2 with sidecar support
* cd out of the GEOPM directory to clone the other repos
* Clone the repo at the v0.4.0 release (https://github.com/kubeflow/mpi-operator):
```
git clone git@github.com:kubeflow/mpi-operator.git -b v0.4.0
```
* Apply the GEOPM enabling patch:
```
cd mpi-operator
git am $GEOPM_CLOUD_BRANCH/service/kubecon-na-23/mpi-operator/0001-feat-k8s-infra-to-support-geopm-batch.patch
```
* Setup the common module:
```
git clone git@github.com:kubeflow/common.git
cd common
git am $GEOPM_CLOUD_BRANCH/service/kubecon-na-23/common/0001-Update-versions-of-common-module-dependencies.patch
cd ..
```
* Build a custom image by running:
```
make RELEASE_VERSION=test10 IMAGE_NAME=localhost:5000/mpi-operator images
```
* Start a local docker container registry if you don't already have one:
```
docker run -d -p 5000:5000 --restart always --name registry registry:2
```
* Add the image:
```
docker push localhost:5000/mpi-operator:test10
```
* Deploy the mpi operator by running the following:
```
kubectl create namespace kubeflow
kubectl apply -k manifests/overlays/kubeflow
kubectl kustomize manifests/base | kubectl apply -f -
```

## Install Kueue
```
VERSION=v0.4.1
kubectl apply -f https://github.com/kubernetes-sigs/kueue/releases/download/$VERSION/manifests.yaml
```

## Configure Kueue Cluster Queue
* Set the queue power limit by changing the "intel.com/power" Watt value in
  "$GEOPM_CLOUD_BRANCH/service/kubecon-na-23/kueue-config/admin/single-clusterqueue-setup.yaml" :
```
    - name: "intel.com/power"
        nominalQuota: 10000
```
* Apply the configuration by running:
```
kubectl apply -f kueue-config/admin/single-clusterqueue-setup.yaml
```

## Build and Start the power capping device plugin.
First edit the `plugin.NewDevicePluginStub(generateDevices(1000)` line in
`main.go` to indicate the maximum requestable Watts per node you want to make
available. Then build and run:
```
cd geopm-dp
make
export PLUGIN_SOCK_DIR=/var/lib/kubelet/device-plugins/
./bin/geopm-dp
```

## Deploy GEOPM DaemonSet
kubectl apply -f ../k8-service.yaml

## Run mpi jobs with kueue:
* Build mnist image for kueue from "mpi-operator/examples/v2beta1/horovod" by running:
```
 docker build -t localhost:5000/tfmnist:0.1 .
 docker push localhost:5000/tfmnist:0.1 .
```

* Deploy the job(change of docker registry might be required):
```
kubectl apply -f kueue-config/jobs/sample-mpijob-mnist.yaml
```

## Evaluate a power sweep on the Cosmic Tagger app
1. Run the app under multiple power caps
```
mkdir -p jobs; for powercap in {600..1200..100}; do sed -e "s/\$POWERCAP/$powercap/g" ./cosmic-tagger-power-sweep.yaml > ./jobs/cosmic-powercap-$powercap.yaml; done
kubectl apply -f jobs
```

2. Extract app logs and geopm-client traces from kubernetes
```
mkdir -p power_sweep
for pod in $(kubectl get pods -ljobgroup=cosmic-tagger -o name)
do
  kubectl logs $pod --container geopm-client > power_sweep/${pod#*/}.csv
  kubectl logs $pod > power_sweep/${pod#*/}.log
done
```

3. Use the power sweep to select a power cap (e.g, for 5% tolerable slowdown).
Optionally specify a --plot-path directory inside which will be saved a plot of
the power sweep)
```
./model_sweep.py power_sweep/*csv --power-at-slowdown 0.05
```

## Uninstall Kueue
```
VERSION=v0.4.1
kubectl delete -f https://github.com/kubernetes-sigs/kueue/releases/download/$VERSION/manifests.yaml
```

## Uninstall mpi-operator
```
cd mpi-operator
kubectl delete -k manifests/overlays/kubeflow
kubectl kustomize manifests/base | kubectl delete -f -
```
