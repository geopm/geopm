GEOPM Container Support
=======================

This directory contains configuration files and scripts to support GEOPM
containerization use cases. The files include scripts for building containers
with Docker and Kubernetes configuration files to orchestrate GEOPM services.


GEOPM Access Service
--------------------

The GEOPM Access Service is typically deployed as a systemd service, however
this service may also be provided as a containerized deployment using
Kubernetes.  In this use case, a container is launched with privilege to provide
the GEOPM Access Service.  This service can be used by unprivileged containers
to read GEOPM metrics, or modify controls during the lifetime of the client
container.


Prometheus GEOPM Exporter Service
---------------------------------

The Prometheus GEOPM Exporter Service uses the GEOPM Access Service to sample
telemetry and publishes aggregated metrics for a Prometheus server to scrape.
The Prometheus client container may be deployed without privilege beyond
configuring the GEOPM Access Service.


Building Docker Containers
--------------------------

A script, `docker-build.sh` is provided that uses Docker to build an Ubuntu
based container that provides the GEOPM software packages.  These packages
provide `geopmd` and `geopmexporter` which are the entry points for the GEOPM
Access Service and the Prometheus GEOPM Exporter Service respectively.  The
build script uses the `geopm-prometheus-pkg.Dockerfile` to create a container
that builds the GEOPM Access Service Ubuntu packages.  These packages are used
by the `geopm-prometheus.Dockerfile` to create a runtime container that can
support the GEOPM services.  The runtime container is tagged "geopm-prometheus".


Deploying Prometheus Client in Kubernetes
-----------------------------------------

After building the container to support the GEOPM Services, you may use the
`geopm-prometheus-k8.yml` Kubernetes manifest to enable the GEOPM Access and
Prometheus GEOPM Exporter Services.  The GEOPM Access services is provided to
client containers over interfaces in the `/run/geopm` mount point that is shared
between the containers in a pod.  The interfaces are serviced by the `geopmd`
process running in a privileged container launched with access to device driver
interfaces.  The Prometheus GEOPM Exporter is provided on port 8000 and gives
access to all power, energy, frequency and thermal metrics that GEOPM discovers
on the platform.  Note that the `geopm-prometheus-k8.yml` manifest may be
modified with any of the `geopmexporter(1)` command line options.



Grafana Dashboard
-----------------

See `geopm/integration/grafana` for an example dashboard that utilizes the
metrics collected by the `geopmexporter(1)`.
