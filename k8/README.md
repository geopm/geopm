
# GEOPM Runtime as a Kubernetes Service

The Kubernetes manifest files in this directory provide some examples that
show the GEOPM Runtime provided as a service.  In each file, a GEOPM Agent
is configured and a GEOPM Controller is launched.  The controller will wait
for an application to connect.

## Status

GEOPM Runtime as a Kubernetes service is a work in progress.  The current
support is a proof-of-concept, and should not be considered production ready.

## The Monitor Agent

The first example manifest ``k8-monitor.yaml`` launches the monitor agent and
any application that links to ``libgeopmload.so`` and has
``GEOPM_PROFILE=monitor`` set in their environment will run with GEOPM
monitoring.  The resulting report is output to the ``geopm-ctl`` container log.
