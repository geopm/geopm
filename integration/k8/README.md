# GEOPM Container Support

This directory contains configuration files and scripts to support GEOPM
containerization. It includes scripts for building containers with Docker and
Kubernetes configuration files for orchestrating GEOPM services.

## GEOPM Access Service

The GEOPM Access Service is typically deployed as a systemd service. However, it
can also be provided as a containerized deployment using Kubernetes. In this
setup, a privileged container provides the GEOPM Access Service. This service
allows unprivileged containers to read GEOPM metrics or modify controls during
their lifetime.

## Prometheus GEOPM Exporter Service

The Prometheus GEOPM Exporter Service uses the GEOPM Access Service to sample
telemetry. It publishes aggregated metrics for a Prometheus server to
scrape. The Prometheus client container does not require elevated privileges
beyond configuring the GEOPM Access Service.

## Building Docker Containers

A script, `docker-build.sh`, is provided to build an Ubuntu-based container with
the GEOPM software packages. These packages include `geopmd` and
`geopmexporter`, which are the entry points for the GEOPM Access Service and the
Prometheus GEOPM Exporter Service, respectively.

The build script uses the `geopm-prometheus.Dockerfile` to create a container
that builds the GEOPM Access Service Ubuntu packages. These packages are then
used to create a runtime container supporting the GEOPM services. The runtime
container is tagged as "geopm-prometheus."

## Deploying Prometheus Client in Kubernetes

After building the container, you can use the `geopm-prometheus-k8.yml`
Kubernetes manifest to enable the GEOPM Access and Prometheus GEOPM Exporter
Services. The GEOPM Access Service provides interfaces in the `/run/geopm` mount
point, shared between containers in a pod. These interfaces are serviced by the
`geopmd` process running in a privileged container with access to device driver
interfaces.

The Prometheus GEOPM Exporter runs on port 8000 and provides metrics such as
power, energy, frequency, and thermal data discovered by GEOPM. You can modify
the `command` in the `geopm-prometheus-k8.yml` manifest to include any
`geopmexporter(1)` command-line options.

The pod is deployed in the `geopm` namespace. Reasonable resource limits are
applied but may need adjustment based on your requirements. While the GEOPM
Access Service requires elevated privileges, the Prometheus exporter container
does not.

Note: The Docker image created by the `docker-build.sh` script must be uploaded
to a registry using the `docker push` command. Update the `image` value in the
`geopm-prometheus-k8.yml` manifest to match the tag in the registry.

### Using Host OS for GEOPM Access

An alternative Kubernetes manifest, `geopm-prometheus-host.yml`, allows the
GEOPM Access Service to be provided by the host OS as a systemd service. This
approach is preferred if the host OS already provides the GEOPM Access Service
or if installing the GEOPM packages on the host is preferred over running a
privileged container.

To enable this, edit the GEOPM systemd unit file using the `systemctl edit
geopm` command. Modify the `ExecStart` field to include the `--grpc` option and
set the `Type` to `simple`:

```
$ sudo systemctl edit geopm
$ cat /etc/systemd/system/geopm.service.d/override.conf
[Service]
Type=notify
ExecStart=
ExecStart=/usr/bin/geopmd --grpc
$ sudo systemctl daemon-reload
$ sudo systemctl restart geopm
```

Additionally, modify the `geopm-prometheus-host.yml` manifest so that the
`image` field points to the tag in your registry. Update the `runAsUser` and
`runAsGroup` fields to reference a user and group ID on the host system (both
are set to 1001 in the example manifest file) that has been granted access to
the required signals for `geopmexporter`:

```
printf \
"CPU_CORE_TEMPERATURE\nCPU_ENERGY\nCPU_FREQUENCY_STATUS\n"\
"CPU_PACKAGE_TEMPERATURE\nCPU_POWER\nCPU_UNCORE_FREQUENCY_STATUS\n"\
"DRAM_ENERGY\nDRAM_POWER\nGPU_CORE_FREQUENCY_STATUS\nGPU_ENERGY\n"\
"GPU_POWER\nGPU_TEMPERATURE\n" | \
    geopmaccess --direct --force --write --group 1001
```

Note: Older versions of the gRPC implementation may hang on x86 systems for the
GEOPM use case. This issue is documented here:
<https://bugs.launchpad.net/ubuntu/+source/grpc/+bug/1971114>. For this reason,
we recommend this modification only for newer host OS distributions, such as
Ubuntu Noble 24.04, where `python3-grpcio` has been updated.

## Grafana Dashboard

Refer to `geopm/integration/grafana` for an example Grafana dashboard that
utilizes the metrics collected by `geopmexporter(1)`.
