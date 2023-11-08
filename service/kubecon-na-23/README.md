# Getting Started

## Install custom mpi-operator for k8s v.1.28.2 with sidecar support
* Build a customer image by running "make RELEASE_VERSION=test2 IMAGE_NAME=localhost:5000/mpi-operator images"
* Change images to your local images in "mpi-operator/manifest/deployment.yaml"
* deploy the mpi operation by running within the mpi-operator folder:
```
kubectl apply -k manifests/overlays/kubeflow
kubectl kustomize base | kubectl apply -f -
```
## Install Kueue
```
VERSION=v0.4.1
kubectl apply -f https://github.com/kubernetes-sigs/kueue/releases/download/$VERSION/manifests.yaml
```
## Configure Kueue Cluster Queue
* Set the queue power limit by changing the "intel.com/power" Watt value in  "kueue-config/admin/single-clusterqueue-setup.yaml" :
```
    - name: "intel.com/power"
        nominalQuota: 10000
```
* Apply the configuration by running "kubectl apply -f kueue-config/admin/single-clusterqueue-setup.yaml"
## Build and Start the power capping device plugin
```
cd geopm-dp
make
export PLUGIN_SOCK_DIR=/var/lib/kubelet/device-plugins/
./bin/geopm-dp
```
## Deploy GEOPM DeamonSet
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

## Uninstall Kueue
```
VERSION=v0.4.1
kubectl delete -f https://github.com/kubernetes-sigs/kueue/releases/download/$VERSION/manifests.yaml
```
## Uninstall mpi-operator
```
kubectl delete -k manifests/overlays/kubeflow
kubectl kustomize base | kubectl delete -f -
```