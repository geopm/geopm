grafana
=======
This directory contains JSON files for `Grafana` dashboards
that display visualizations of metrics exported by a Prometheus
data source. 


Importing the Grafana dashboard
-------------------------------
Follow the steps documented in the official Grafana user guide
<https://grafana.com/docs/grafana/latest/dashboards/build-dashboards/import-dashboards/>
to import any of the JSON files in this directory for generating
the preferred visualizations. 


Data Source for the Visuals
---------------------------
Grafana is capable of sourcing telemetry data from Prometheus
clients running on remote hosts. One such Prometheus exporter
is the `geopmexporter(1)` 
<https://geopm.github.io/geopmexporter.1.html> which leverages
`geopm.stats.collector` to summarize the metrics. The dashboard
file - `GEOPM_Report.grafana.dashboard.json`, in this directory, 
relies on `geopmexporter(1)` for its data source.

