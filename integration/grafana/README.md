# grafana

This directory contains JSON files for `Grafana` dashboards
that display visualizations of metrics exported by a Prometheus
data source. 


## Importing the Grafana dashboard

Follow the steps documented in the official Grafana user guide
<https://grafana.com/docs/grafana/latest/dashboards/build-dashboards/import-dashboards/>
to import any of the JSON files in this directory for generating
the preferred visualizations. 


## Data Source for the Visuals

Grafana is capable of sourcing telemetry data from Prometheus
clients running on remote hosts. One such Prometheus exporter
is the `geopmexporter(1)` 
<https://geopm.github.io/geopmexporter.1.html> which leverages
`geopm.stats.collector` to summarize the metrics. The dashboard
file - `GEOPM_Report.grafana.dashboard.json`, in this directory, 
relies on `geopmexporter(1)` for its data source.


## Use case: Monitoring PBS jobs

The `geopmexporter(1)` is typically deployed system-wide with either a systemd
service or as a kubernetes service.  This documentation is for an alternative
`geopmexporter(1)` deployment scenario where an unprivileged user of a PBS
managed system would like to monitor the compute resources granted to the user
only while they are allocated.  This methodology should not be applied for
system-wide monitoring by an administrator, but rather for job monitoring by an
end user of a PBS system where the geopmexporter is not deployed system-wide.

A script is provided called `pbs_prometheus_launch.sh` that can be used to:
(a) assist with the initial one-time configuration of the prometheus & grafana
servers on the head node.
(b) monitor jobs submitted to a PBS batch system.

The script runs the prometheus and grafana server on the head node of a system,
and will connect to `geopmexporter(1)` clients running on the allocated nodes 
of a user's job.


### Requirements

A user installation of grafana and prometheus is required.  We recommend
following the installation instructions
[here](https://grafana.com/grafana/download?platform=linux) to `wget` the
grafana archive for Linux.  The expanded archive is the `GRAFANA_DIR` provided
to the `pbs_prometheus_launch.sh` script CLI.  To install prometheus download
the archive for the latest stable Linux release from
[here](https://prometheus.io/download/).  The expanded archive is the
`PROMETHEUS_DIR` provided to the `pbs_prometheus_launch.sh` script CLI.

Until the `geopmexporter(1)` is available in a stable release, the user must
build and install a development snapshot of GEOPM.

```bash
    # Install latest development snapshot of GEOPM
    GEOPM_BUILD=... # target installation directory
    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
    chmod a+x install_user.sh
    ./install_user.sh --prefix=$GEOPM_BUILD --enable-levelzero
```


### Configure Prometheus & Grafana

Before deploying the GEOPM Prometheus exporter across the system for the
first time, the prometheus & grafana servers need to be configured on the
head node. The main steps to configure prometheus & grafana are:
[A] Download and untar the prometheus & grafana builds
[B] Configure the prometheus server with the port numbers over which
the exported metrics are expected to stream in from the target nodes
being monitored.
[C] Launch the prometheus server over a desired port over which the grafana server
is expected to query the aggregated metrics from.
[D] Configure the port number for the grafana server over which the end user is expected
to launch the web GUI.
[E] Set the grafana credentials using which the end user is expected to visualize the Web UI
[F] Download & import the grafana dashboard configuration file.
[G] Terminate the prometheus & grafana server processes, post configuration.


Steps-B through D can be achieved by using the `pbs_prometheus_launch.sh` script provided in
the sane directory as this README. 
```bash
    # First, identify the location of prometheus & grafana builds, the
    # preferred ports for their servers, and the grafana server admin password

    PROMETHEUS_DIR=...         # Path to prometheus build on head node
    PROMETHEUS_SERVER_PORT=... # Preferred port for the prometheus server
    GRAFANA_DIR=...            # Path to grafana build on head node
    GRAFANA_SERVER_PORT=...    # Preferred port for the grafana server


    # Next, use the launch script by passing the paths & ports identified
    # in Step-1 as parameters

    cd $GEOPM_SRC/integration/grafana/
    ./pbs_prometheus_launch.sh $PROMETHEUS_DIR $PROMETHEUS_SERVER_PORT \
                               $GRAFANA_DIR $GRAFANA_SERVER_PORT
```

Step-E requires you to set the administrator password for your grafana 
instance with the `grafana-cli` tool.
```bash
    cd $GRAFANA_DIR/
     ./bin/grafana-cli admin reset-admin-password $GRAFANA_ADMIN_PASSWORD
```

Step-F requires you to download and import the grafana dashboard configuration
file using the Web UI. The dashboard configuration file is available here:
```bash
    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/integration/grafana/GEOPM_Report.grafana.dashboard.json
```
Import the GEOPM grafana dashboard with grafana web GUI running on
`http://SERVER_NAME:GRAFANA_SERVER_PORT`. When importing the GEOPM dashboard use the 
prometheus server running on `http://SERVER_NAME:PROMETHEUS_SERVER_PORT` as the datasource.


Step-G: Be sure to kill the prometheus and grafana servers after the configuration is
compleed. If needed the PIDs for termination are printed by the `pbs_prometheus_launch.sh`
script after it launches the prometheus & grafana 
```bash
    pkill prometheus
    pkill grafana
```


### Monitor the user job using the GEOPM Prometheus & Grafana framework
Now that you have configured Grafana and Prometheus, you can launch the prometheus
client exporters across the allocated nodes of a job, and then use the grafana Web GUI
(running on port `GRAFANA_SERVER_PORT` to monitor the aggregated telemetry. 

```bash
    JOBID=$(qsub ...)
    ./pbs_prometheus_launch.sh PROMETHEUS_DIR PROMETHEUS_SERVER_PORT GRAFANA_DIR GRAFANA_SERVER_PORT [GEOPM_DIR JOBID PROMETHEUS_CLIENT_PORT]
```
