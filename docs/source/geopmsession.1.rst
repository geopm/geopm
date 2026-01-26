geopmsession(1) -- sample platform information over time
========================================================

Synopsis
--------

.. code-block:: bash


  usage: geopmsession [-h] [-v] [-t TIME] [-p PERIOD] [--pid PID]
                      [--print-header | -n] [-d DELIMITER]
                      [-r REPORT_OUT] [-o TRACE_OUT] [-f REPORT_FORMAT]
                      [-s REPORT_SAMPLES] [-i CONFIG_PATH] [-a]
                      [--daemon DAEMON_PID_FILE | --enable-mpi]
                      [-- LAUNCH ...]

Read a signal
~~~~~~~~~~~~~

.. code-block:: bash

    echo "SIGNAL_NAME DOMAIN DOMAIN_IDX" | geopmsession

Read a signal at a specific period for a specific timeout
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmsession -p PERIOD_IN_SECONDS -t TIMEOUT_IN_SECONDS
    geopmsession --period PERIOD_IN_SECONDS --time TIMEOUT_IN_SECONDS

Read a set of signals
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    echo -e 'TIME board 0\nCPU_FREQUENCY_STATUS package 0' | geopmsession -n

Get Help or Version
~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmsession -h
    geopmsession --help


Description
-----------

Command line interface for the GEOPM service batch read features. The input to
the command line tool has one request per line. A request for reading is made up
of three strings separated by white space. The first string is the signal name,
the second string is the domain name, and the third string is the domain index.
Provide the "``*``" character as the second string to request the native domain
for the signal.  Provide the "``*``" character as the third string to request
all domains available on the system.

A descriptive header is written first, unless the ``-n`` option is specified, in
which case the header omitted.  The output from reading values is printed
subsequently in CSV format.  By default, only one line of CSV will be generated.
Use ``-p`` to create a CSV with multiple rows providing a time series of
measurements.


Options
-------

-h, --help  .. _help option:

    Print help message and exit.

-v, --version  .. _version option:

    Print version and exit.

-t TIME, --time TIME  .. _time option:

    Total run time of the session to be opened in seconds.

-p PERIOD, --period PERIOD  .. _period option:

    When used with a read mode session reads all values out periodically with
    the specified period in seconds.  Default: 0.1 second.

--pid PID  .. _pid option:

    Stop the session when the given process PID ends.

--print-header  .. _printheader option:

    Deprecated. Now this option is the default, see --no-header.

-n, --no-header  .. _header option:

    Do not print the CSV header before printing any sampled values.

-d, --delimiter  .. _delimiter DELIMITER .. option:

    Delimiter used to separate values in CSV output. Default: ",".

-r, --report-out  .. _reportout REPORT_OUT option:

    Output summary statistics into a yaml file. Note if ``--report-out=-`` is
    specified, the report will output to stdout. When used with the
    ``--enable-mpi`` option, reports from all hosts will be combined using the
    ``---`` document separator, and the output is written (stdout or to file) solely
    by the MPI process "rank 0".

-o, --trace-out  .. _traceout TRACE_OUT option:

    Output trace data into a CSV file. Note if ``--trace_out=-`` is specified,
    the trace will output to stdout which is also the default behavior. To avoid
    gathering trace data, set this parameter to ``/dev/null``.  When used with
    the ``--enable-mpi`` option, trace file names will be appended with the
    hostname combined with the ``-`` separator.  It is not possible to write the
    trace output to stdout when specifying ``--enable-mpi``, this will result in
    an error.

-f, --report-format  .. _reporformat REPORT_FORMAT option:

    Generate reports in the specified format, either "csv" or "yaml".
    Default: "yaml".

-s, --report-samples  .. _reportsamples REPORT_SAMPLES option:

    Create reports each time the specified number of periods have
    elapsed.  When in YAML format, the reports are YAML documents
    separated with the document separator string: ``"---"``.  When
    in CSV format, each report is one line of the CSV output.

-i, --signal-config  .. _signalconfig SIGNAL_CONFIG option:

    Input file containing GEOPM signal requests, specify "-" to use
    standard input which is also the default.

-c, --control-config  .. _controlconfig CONTROL_CONFIG option:

    Apply control settings from the specified configuration file before
    sampling begins. Each non-empty line must contain four whitespace
    separated fields: ``CONTROL_NAME DOMAIN_TYPE DOMAIN_INDEX VALUE``. The
    format matches the batch files accepted by :doc:`geopmwrite(1)
    <geopmwrite.1>`. Controls are written once at startup and restored when
    the session exits, making it possible to pin power caps or other knobs
    for the duration of the run.

--daemon  .. _daemon DAEMON_PID_FILE option:

    Run geopmsession as a background daemon. The daemon PID is written to the
    specified file after startup and the invoking command returns immediately
    once the session is ready. A signal configuration file must be provided
    (standard input is not supported when using this option). Using
    ``--enable-mpi`` is not allowed when specifying this option, consider
    using ``--append-hostname`` instead.

-a, --append-hostname  .. _appendhostname option:

    Append the local hostname to report and trace file paths when they are
    regular files. This keeps per-host outputs unique on shared filesystems.

--enable-mpi  .. _enablempi option:

    Gather reports over MPI and write to a single file. Append hostname to trace
    output file if specified (trace output to stdout not permitted). Requires
    mpi4py module.


Launch Option
-------------
The ``geopmsession`` tool may be used to launch and monitor a subprocess, terminating
the session when the process exits. To use this feature, provide a command after
a double dash (``--``). For example:

.. code-block:: bash

   $ echo TIME board 0 | geopmsession -p 1 -- sleep 5
   "TIME"
   0.001785301
   1.008901696
   2.008939004
   3.009074361
   4.009136714
   5.009231953
   6.009308985

This will launch ``sleep 5`` as a subprocess and monitor the TIME signal until
the process exits. Using the ``--pid`` and the launch option at the same
time is forbidden.

If the geopmsession process receives a SIGTERM and SIGINT or fails dues to an
unmanaged exception, the signal is forwarded to the subprocess and all of its
children followed by SIGKILL after 1 second.  If the geopmsession command fails
due to an exception then the first signal sent is SIGINT.

If using ``--enable-mpi`` with the launch option, note that each MPI rank will
launch its own subprocess, which may not be the intended behavior for MPI
applications.


Agent Support
-------------
The ``geopmsession`` tool supports Python agent plugins that can customize session
behavior. An agent is a Python class derived from the ``Agent`` base class in
``geopmdpy.session``. Agents can add custom command-line arguments, override the
default signal configuration, and provide additional trace columns.  The agent
can also implement custom logic for each sampling period including the ability
to modify control knobs dynamically. The agent implementation uses the ``main()``
function from the ``geopmdpy.session`` module as the entry point.


Examples
--------

Some examples of how to use the ``geopmsession`` command line tool are
provided.

Reading a signal
~~~~~~~~~~~~~~~~
The input to the command line tool has one request per line.  A
request for reading is made of up three strings separated by white
space.  The first string is the signal name, the second string is the
domain name, and the third string is the domain index.  An asterisk ``*``
in place of the domain name will evaluate to the native domain of the signal.
An asterisk ``*`` in place of the domain index will result in the signal being
read for all available domain indices on the system for the specified domain type.

An example where the entire ``THERM_STATUS`` model specific register is read from
core zero:

.. code-block:: shell-session

   $ echo "MSR::THERM_STATUS# core 0" | geopmsession -n
   0x0000000088430800

This will execute one read of the signal.

A couple of examples reading ``CPU_POWER`` using ``*``:

.. code-block:: shell-session

   $ echo "CPU_POWER * 1" | geopmsession -n
   62.95697469659621
   $ echo "CPU_POWER * *" | geopmsession -n
   69.16105633883998,74.9419453995438

Reading a signal periodically
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Both a polling period and timeout must be specified.
The polling period must be shorter than the timeout specified.

A 100ms polling period with a 300ms timeout is shown below:

.. code-block:: shell-session

   $ echo 'MSR::THERM_STATUS# core 0' | geopmsession -p 0.1 -t 0.3 -n
   0x0000000088350800
   0x0000000088350800
   0x0000000088350800
   0x0000000088360800


Reading a set of signals
~~~~~~~~~~~~~~~~~~~~~~~~

Multiple signals may be specified by separating them with a newline.

.. code-block:: shell-session

   $ printf 'TIME board 0\nCPU_FREQUENCY_STATUS package *\nCPU_ENERGY package *\n' > session.config
   $ geopmsession -i config.txt
   "TIME","CPU_FREQUENCY_STATUS-package-0","CPU_FREQUENCY_STATUS-package-1","CPU_ENERGY-package-0","CPU_ENERGY-package-1"
   0.658525605,1000000000,1105000000,51985.06903076172,216490.1282958984

Signals may be specified in a separate file using the ``-i`` option.


Reading a set of signals and getting summary statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Summary statistics may be output to stdout by setting ``--report-out=-``.
Otherwise, the statistics will be output to the specified file path. If
unspecified, no statistics will be gathered.

The resulting report will be in yaml by default. To output as a csv, use the
``-f csv`` option. Hostname and sample information will be output at the top.
Summary statistics (count/first/last/min/max/mean/std) will be output
for each of the specified signals at the specified domains/domain indices.

.. code-block:: shell-session

    $ printf 'TIME board 0\nCPU_POWER board 0\nCPU_FREQUENCY_STATUS board 0\n' |\
        geopmsession -t 10 -p 0.005 --report-out=- --trace-out=/dev/null

An example yaml report is shown below:

.. code-block:: yaml

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:08:53.160991796-0700"
   sample-time-total: 10.0013
   sample-count: 2001
   sample-period-mean: 0.00500067
   sample-period-std: 0.000494084
   metrics:
     TIME:
       count: 2001
       first: 1.13225
       last: 11.1336
       min: 1.13225
       max: 11.1336
       mean: 6.13339
       std: 2.88899
     CPU_POWER:
       count: 2001
       first: 81.3372
       last: 114.556
       min: 81.3372
       max: 145.638
       mean: 117.871
       std: 7.55413
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 1.0775e+09
       last: 1.0375e+09
       min: 1e+09
       max: 1.3325e+09
       mean: 1.07005e+09
       std: 3.80748e+07


The same report rendered into csv format:

.. code-block:: text

   "host","sample-time-first","sample-time-total","sample-count","sample-period-mean","sample-period-std","CPU_FREQUENCY_STATUS-count","CPU_FREQUENCY_STATUS-first","CPU_FREQUENCY_STATUS-last","CPU_FREQUENCY_STATUS-min","CPU_FREQUENCY_STATUS-max","CPU_FREQUENCY_STATUS-mean","CPU_FREQUENCY_STATUS-std","CPU_POWER-count","CPU_POWER-first","CPU_POWER-last","CPU_POWER-min","CPU_POWER-max","CPU_POWER-mean","CPU_POWER-std","TIME-count","TIME-first","TIME-last","TIME-min","TIME-max","TIME-mean","TIME-std"
   "cluster-node-11","2025-05-16T09:09:16.559516035-0700",10.000596301,2001,0.0050002981505,0.0004777162996813195,2001,1085000000.0,1172500000.0,1000000000.0,1282500000.0,1071716016.9915042,38699360.87433021,2001,134.5205438626066,132.45502415239116,104.55274444442776,148.3458434284852,119.53508979088977,5.258855212642797,2001,1.574881395,11.575477696,1.574881395,11.575477696,6.576009187958503,2.888976620038315


Launching a process and monitoring signals
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: shell-session

   $ echo TIME board 0 | geopmsession -p 0.5 -- sleep 5
   "TIME"
   0.005026064
   0.513985594
   1.013916775
   1.513919136
   2.013917347
   2.513900635
   3.0139143
   3.513901374
   4.013907436
   4.513900239
   5.013895239
   5.513876408

This launches ``sleep 5`` and monitors the TIME signal until it detects that the
``sleep`` command has exited.

.. _geopmsession.1:reading-signals-during-a-job-execution:

Reading signals during a job execution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Signals can be read and summary statistics gathered during job execution using
the launch or ``--pid`` option. If both the ``--pid`` and ``-t`` options are used,
geopmsession will end when either the process ends or when the specified time
elapses, whichever is shorter. Below is an example gathering ``CPU_POWER``
while running ``sleep``.


.. code-block:: shell-session

   $ echo "CPU_POWER package 0" | geopmsession -p 1 -- sleep 5
   "CPU_POWER-package-0"
   62.54857725485511
   50.5686841290089
   57.63407322142274
   61.1508168939381
   60.71903156269835
   59.50978494274343
   58.25752970206295


.. code-block:: shell-session

   $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" | geopmsession --pid $apppid -p 1
      [1] 3100339
      "CPU_POWER-package-0"
      30.59618629083503
      40.73598509986576
      38.79754643472621
      39.32981634681103
      38.70242242812028
      [1]+  Done                    sleep 5

An example gathering summary statistics while executing a job:

.. code-block:: shell-session

   $ echo "CPU_POWER package 0" | geopmsession -p 1 -r - -- sleep 5
   "CPU_POWER-package-0"
   39.15323231457158
   39.9597792259686
   40.38713156571379
   40.3391948981358
   40.10173665761857
   39.93951912781292
   39.83074165703577
   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:19:42.253776983-0700"
   sample-time-total: 6.00905
   sample-count: 7
   sample-period-mean: 1.00151
   sample-period-std: 0.00372384
   metrics:
     CPU_POWER-package-0:
       count: 7
       first: 39.1532
       last: 39.8307
       min: 39.1532
       max: 40.3871
       mean: 39.9588
       std: 0.411159


Note that the samples are output followed by summary statistics. To output the
sample trace to a file, use ``-o [filename]``. To output the summary statistics
report to a file, use ``-r [filename]``. To suppress the trace, set the output
parameter to ``-o /dev/null``. Reports will not output if ``-r`` is not specified.

Using the ``-s [REPORT_SAMPLES]`` option will generate statistics after the
specified number of samples. In default yaml format, sets of statistics will
be separated by "---". In csv format, each set of statistics will be output as
a row.

Example:

.. code-block:: shell-session

     $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" |\
       geopmsession --pid $apppid -p 0.1 -r - -o /dev/null -s 10

In the yaml output below, note that each report is appended, separated by
"---".

.. code-block:: yaml

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:21:12.854647716-0700"
   sample-time-total: 1.0005
   sample-count: 11
   sample-period-mean: 0.10005
   sample-period-std: 0.00017891
   metrics:
     CPU_POWER-package-0:
       count: 11
       first: 26.1087
       last: 41.1297
       min: 26.1087
       max: 42.36
       mean: 37.5301
       std: 4.85142

   ---

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:21:13.955151931-0700"
   sample-time-total: 0.899985
   sample-count: 10
   sample-period-mean: 0.0999983
   sample-period-std: 7.34092e-05
   metrics:
     CPU_POWER-package-0:
       count: 10
       first: 39.0531
       last: 39.4301
       min: 35.197
       max: 40.6032
       mean: 38.1963
       std: 2.09879

   ---

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:21:14.955165125-0700"
   sample-time-total: 0.899977
   sample-count: 10
   sample-period-mean: 0.0999974
   sample-period-std: 5.01953e-05
   metrics:
     CPU_POWER-package-0:
       count: 10
       first: 37.8657
       last: 41.0475
       min: 36.256
       max: 44.8203
       mean: 40.6383
       std: 3.36432

   ---

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:21:15.955163708-0700"
   sample-time-total: 0.600004
   sample-count: 7
   sample-period-mean: 0.100001
   sample-period-std: 0.000118453
   metrics:
     CPU_POWER-package-0:
       count: 7
       first: 38.3654
       last: 39.4388
       min: 34.8214
       max: 39.4388
       mean: 36.8331
       std: 1.82103


Sample csv output below shows each statistics sample output on a new row:

.. code-block:: shell-session

   $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" |\
   geopmsession --pid $apppid -p 0.1 -r - -o /dev/null -s 10 -f csv
   "host","sample-time-first","sample-time-total","sample-count","sample-period-mean","sample-period-std","CPU_POWER-package-0-count","CPU_POWER-package-0-first","CPU_POWER-package-0-last","CPU_POWER-package-0-min","CPU_POWER-package-0-max","CPU_POWER-package-0-mean","CPU_POWER-package-0-std"
   "cluster-node-11","2025-05-16T09:23:33.620790309-0700",1.000574921,11,0.10005749209999999,0.0001964757920469247,11,39.44969148140212,41.9486178974159,34.84014539393113,41.9486178974159,36.49265394626341,2.465552738326359
   "cluster-node-11","2025-05-16T09:23:34.721404331-0700",0.899903938,10,0.09998932644444444,3.917289581063831e-05,10,43.77618495751115,39.447376090215,35.07951019074365,44.37725410303373,39.60711256469672,3.6046116965994814
   "cluster-node-11","2025-05-16T09:23:35.721373075-0700",0.8999950860000001,10,0.09999945400000002,0.00010700698475670486,10,40.55431660299817,39.64840048680215,34.56523283529788,40.81876798613059,38.042165265899925,2.384395315318058
   "cluster-node-11","2025-05-16T09:23:36.721240770-0700",0.6000795879999998,7,0.10001326466666664,7.467743569208487e-05,7,40.97335095740563,34.912491389496914,34.912491389496914,41.4496234018482,38.75680603826903,2.716795748688028


Gathering Reports using MPI
---------------------------

The ``--enable-mpi`` command line option can be used to aggregate reports using
an MPI communicator.  This can be helpful when running sessions on more than one
compute node in an MPI enabled environment.  The user must install the optional
``mpi4py`` package to use the ``--enable-mpi`` command line option .  This can
be done using the OS package manager or PyPi.  When running in this way the
``geopmsession`` command line tool must be launched with an MPI launch wrapper
like ``mpiexec`` or ``mpirun``.  The user should run this command specifying one
``geopmsession`` process per compute node.  When using this option, trace output
to stdout is disabled.  The aggregated report is created by the "rank 0" process
of the geopmsession MPI communicator.


.. code-block:: shell-session

   $ printf "TIME board 0\nCPU_POWER board 0\nCPU_FREQUENCY_STATUS board 0" |\
        srun -n 2 -N 2 geopmsession -t 10 -p 0.005  -r- -o /dev/null --enable-mpi

An example report is shown below:

.. code-block:: yaml

   host: "cluster-node-11"
   sample-time-first: "2025-05-16T09:26:37.114177564-0700"
   sample-time-total: 10.0011
   sample-count: 2001
   sample-period-mean: 0.00500056
   sample-period-std: 0.000493951
   metrics:
     TIME:
       count: 2001
       first: 0.668767
       last: 10.6699
       min: 0.668767
       max: 10.6699
       mean: 5.66989
       std: 2.88899
     CPU_POWER:
       count: 2001
       first: 84.186
       last: 117.827
       min: 77.7429
       max: 141.491
       mean: 118.953
       std: 7.03585
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 1.04375e+09
       last: 1.035e+09
       min: 1e+09
       max: 1.25625e+09
       mean: 1.07255e+09
       std: 3.91874e+07

   ---

   host: "cluster-node-12"
   sample-time-first: "2025-05-16T09:26:37.112549059-0700"
   sample-time-total: 10.0012
   sample-count: 2001
   sample-period-mean: 0.00500062
   sample-period-std: 0.000465038
   metrics:
     TIME:
       count: 2001
       first: 1.10684
       last: 11.1081
       min: 1.10684
       max: 11.1081
       mean: 6.10793
       std: 2.88901
     CPU_POWER:
       count: 2001
       first: 76.1151
       last: 118.133
       min: 76.1151
       max: 142.527
       mean: 119.894
       std: 7.05624
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 1.045e+09
       last: 1.045e+09
       min: 1e+09
       max: 1.28e+09
       mean: 1.07346e+09
       std: 4.24776e+07


Writing a Custom Agent
----------------------

The ``geopmsession`` command line tool supports Python agent plugins that can
customize session behavior. This example shows a simple agent that monitors the
``CPU_POWER`` signal at high or low resolution.

.. code-block:: python

   #!/usr/bin/env python
   # File: cpu_power_agent.py
   from geopmdpy.session import main
   from geopmdpy.session import Agent

   class CPUPowerAgent(Agent):
       """Agent for monitoring CPU power.

       The CPUPowerAgent provides a --hi-res option to read CPU power at the
       finest granularity available. This allows users to measure CPU power from
       all domains and indices. By default, CPU power is sampled at the board
       domain.

       Command-line options:
         --hi-res   Measure at finest granularity (all domains/indices).

       Example:
           python3 cpu_power_agent.py --hi-res -- sleep 5
       """
       def __init__(self):
           self._hi_res = False

       def help(self):
           return 'Measure CPU_POWER as default signal configuration'

       def update_parser(self, parser):
           parser.add_argument('--hi-res', action='store_true',
                               help='Measure power at finest granularity (all domains/indices)')
           return parser

       def update_args(self, args):
           self._hi_res = args.hi_res
           return args

       def signal_config_override(self):
           if self._hi_res:
               return "CPU_POWER * *"
           else:
               return "CPU_POWER board 0"

   if __name__ == '__main__':
       main(CPUPowerAgent())


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
