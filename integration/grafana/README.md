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


## Monitoring PBS jobs

The `geopmexporter(1)` is typically deployed system wide with either a systemd
service or as a kubernetes service.  This documentation is for an alternative
`geopmexporter(1)` deployment scenario where an unprivileged user of a PBS
managed system would like to monitor the compute resources granted to the user
only while they are allocated.  This methodology should not be applied for
system-wide monitoring by an administrator, but rather for job monitoring by an
end user of a PBS system where the geopmexporter is not deployed system-wide.

A script is provided called `pbs_prometheus_launch.sh` that can be used to
monitor jobs submitted to a PBS batch system.  The script runs the prometheus
and grafana server on the head node of a system, and will connect to
`geopmexporter(1)` clients running on the compute nodes of a user's job.

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

```

    # Install latest development snapshot of GEOPM
    GEOPM_DIR=/mnt/scratch/$USER/geopm-build
    wget https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/geopmdpy/install_user.sh
    chmod a+x install_user.sh
    ./install_user.sh --prefix=$GEOPM_DIR --enable-levelzero

```

### Configure Grafana

Before deploying the GEOPM exporter for the first time, the grafana dashboard
must be configured using the Grafana web GUI.  This requires you to set the
administrator password for you Grafana instance with the `grafana-cli` tool
first.  This password can be used to login to the Grafana web server that is
started by the `pbs_prometheus_launch.sh` script on localhost port 8000.  Be
sure to kill the prometheus and grafana servers after the configuration is
complete: instructions to do so are printed by the script.

```
    # Set up GEOPM Dashboard
    PROMETHEUS_DIR=/mnt/scratch/$USER/prometheus-2.53.3.linux-amd64
    GRAFANA_DIR=/mnt/scratch/$USER/grafana-v11.3.1
     GRAFANA_ADMIN_PASSWORD=GRAFANA_ADMIN_PASSWORD # Choose your own password
    sed 's|http_port = [0-9]*|http_port = 8000|' -i $GRAFANA_DIR/conf/defaults.ini

    ./pbs_prometheus_launch.sh $PROMETHEUS_DIR $GRAFANA_DIR
    # Before using grafana for the first time set administrator password to
    # enable web login
     $GRAFANA_DIR/bin/grafana-cli admin reset-admin-password $GRAFANA_ADMIN_PASSWORD

    # Download GEOPM grafana dashboard configuration here:
    #   https://raw.githubusercontent.com/geopm/geopm/refs/heads/dev/integration/grafana/GEOPM_Report.grafana.dashboard.json
    #   Install the GEOPM grafana dashboard with grafana web GUI running on
    #   localhost:8000.  When importing the GEOPM dashboard use the prometheus
    #   server running on localhost:8001 as the datasource.

    # When done configuring grafana, kill the servers (PIDs are printed by script)
    pkill prometheus
    pkill grafana

```

Now that you have configured Grafana and Prometheus, you can launch a job and monitor it.

```
    JOBID=$(qsub ...)
    ./pbs_prometheus_launch.sh $PROMETHEUS_DIR $GRAFANA_DIR $GEOPM_DIR $JOBID
```
