
geopmexporter(1) -- Prometheus exporter for GEOPM metrics
=========================================================

Synopsis
--------

.. code-block:: none

    geopmexporter [-h] [-v] [-t PERIOD] [-p PORT] [-i CONFIG_PATH] [--summary SUMMARY]


Description
-----------

Prometheus exporter client that publishes telemetry read through GEOPM.  By
default, this exports all power, energy, frequency and thermal metrics that are
available through GEOPM on the platform.  By using the ``-i`` option, the
exporter may be configured to publish any set of available signals.

The metrics are sampled periodically through the GEOPM PlatformIO batch
interface.  The period of these samples defaults to 100 ms, but the user may
override this with the ``-t`` option.  The values measured are summarized when
the Prometheus server scrapes the data published by the exporter.  These data
are published on port 8000 by default, but the user may override this with the
``-p`` option.


Options
~~~~~~~

-h, --help  .. _help option:

    Print help message and exit

-v, --version  .. _version option:

    Print version and exit

-t, --period  .. _period PERIOD option:

    Sample period for fast loop in seconds. Default: 0.1

-p, --port  .. _port PORT option:

    Port to publish Prometheus metrics. Default: 8000

-i, --signal-config  .. _signalconfig CONFIG_PATH option:

    Signal configuration file path, specify "-" to use standard input.  The
    format of the configuration file is described below.

--summary  .. _summary SUMMARY option:

    Summary method, one of "geopm", or "prometheus". Default: geopm


Configuration File
------------------

The input file is a list of read requests each on its own line.  A request for
reading is made up of three strings separated by white space. The first string
is the signal name, the second string is the domain name, and the third string
is the domain index.  Provide the "``*``" character as the third string to
request all domains available on the system.


GEOPM Summary Method
--------------------

There are two options available for summarizing the statistics for the
Prometheus exporter.  The default option is to use the GEOPM ``StatsCollector``
interface to summarize sampled values.  This is a high performance option that
provides fine grained summary statistics through a Prometheus Gauge metric
interface:

count:
   Number of samples recorded since last Prometheus server scrape

first:
   First value sampled after last Prometheus server scrape

last:
   Most recently sampled value prior to Prometheus server scrape

min:
   Smallest sampled value since last Prometheus server scrape

max:
   Largest sampled value since last Prometheus server scrape

mean:
   Sum of all samples divided by the number of samples since last Prometheus
   server scrape

std:
   Sample standard deviation represented by the following sum over samples:

       ``sqrt(sum((x_i - m) ^ 2) / (n - 1))``

   where ``m`` is the mean, ``x_i`` are the samples, and ``n`` is the number of
   samples


Prometheus Summary Method
-------------------------

When the user specifies the ``--summary prometheus`` command line option the
``Summary`` and ``Counter`` metrics built into Prometheus are used to aggregate
samples over time.  In this way the batch of signals read from the GEOPM is used
to update a ``Summary`` metric for GEOPM signals with variable behavior, or a
``Counter`` metric for signals with monotone behavior.  Each metric provides
slightly different statistics for the Prometheus server on the port opened by
``geopmexporter``.  The ``Summary`` metric exports the **count** and **sum**
statistics, where as the ``Counter`` metric exports the **count** and **total**
statistics.  GEOPM calls into the Prometheus metrics through a Python interface.
This means that the quartile metrics are not currently supported for ``Summary``
metrics. See the Prometheus documentation for more information about the
``Summary`` and ``Counter`` metrics and the statistics that they export to the
Prometheus server.

Using Prometheus to aggregate samples provides an easy method to compare against
the GEOPM ``StatsCollector`` method for summary.  Using the ``--summary prometheus``
mode provides a more course grained view of the metrics gathered, and also tends
to have a higher system overhead.


Systemd Service
---------------

One simple way to manage the GEOPM Prometheus exporter process is to create a
systemd service.  To use ``geopmexporter`` with systemd create a systemd unit
configuration file called ``geopmexporter.service`` with the following content:

.. code-block:: ini

   [Unit]
   Description=Prometheus GEOPM Exporter
   StartLimitIntervalSec=0
   StartLimitBurst=3
   Wants=geopm.service
   After=geopm.service
   After=network-online.target

   [Service]
   Environment=PYTHONUNBUFFERED=true
   User=prometheus
   Group=prometheus
   Type=simple
   ExecStart=/usr/bin/geopmexporter
   SyslogIdentifier=geopmexporter
   Restart=always
   RestartSec=1
   StartLimitInterval=0

   [Install]
   WantedBy=multi-user.target


Execute the commands below to install this systemd service and enable it.  These
commands create a user and group called ``prometheus`` which will run the
service, the systemd unit file described above is installed, and then systemd is
notified of the new service.

.. code-block:: bash

   sudo groupadd --system prometheus
   sudo useradd -s /sbin/nologin --system -g prometheus prometheus
   sudo cp geopmexporter.service /etc/systemd/system/
   sudo systemctl deamon-reload
   sudo systemctl enable geopmexporter
   sudo systemctl start geopmexporter

Note to customize the exporter to fit the needs of your system, the
``ExecStart`` command in the systemd unit file may be modified with any of the
command line options described in the options section above.

Exit Status
-----------

The ``geopmexporter`` command will return 0 upon success and -1 on failure.  For
all failures, an error message describing the failure will be printed.  Setting
the ``GEOPM_DEBUG`` environment variable will enable more verbose error
messages.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmsession(1) <geopmsession.1>`,
:doc:`geopm_pio(7) <geopm_pio.7>`
