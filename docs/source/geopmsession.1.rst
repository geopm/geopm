geopmsession(1) -- sample platform information over time
========================================================


Synopsis
--------

.. code-block:: bash

   usage: geopmsession [-h] [-v] [-t TIME] [-p PERIOD] [--pid PID]
                       [--print-header | -n] [-d DELIMITER] [-r REPORT_OUT]
                       [-o TRACE_OUT] [--enable-mpi] [-f REPORT_FORMAT]
                       [-s REPORT_SAMPLES] [-i CONFIG_PATH]

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

--enable-mpi  .. _enablempi option:

    Gather reports over MPI and write to a single file. Append hostname to trace
    output file if specified (trace output to stdout not permitted). Requires
    mpi4py module.

-f, --report-format  .. _reporformat REPORT_FORMAT option:

    Generate reports in the specified format, either "csv" or "yaml".
    Default: "yaml".

-s, --report-samples  .. _reportsamples REPORT_SAMPLES option:

    Create reports each time the specified number of periods have
    elapsed.  When in YAML format, the reports are YAML documents
    separated with the document separator string: ``"---"``.  When
    in CSV format, each report is one line of the CSV output.

-i, --signal-config  .. _configpath CONFIG_PATH option:

    Input file containing GEOPM signal requests, specify "-" to use
    standard input which is also the default.

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
    173.4394938352482
    $ echo "CPU_POWER * *" | geopmsession -n
    302.1005171817655,218.0933036104828

Reading a signal periodically
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Both a polling period and timeout must be specified.
The polling period must be shorter than the timeout specified.

A 100ms polling period with a 300ms timeout is shown below:

.. code-block:: shell-session

    $ echo 'MSR::THERM_STATUS# core 0' | geopmsession -p 0.1 -t 0.3 -n
    0x0000000088410000
    0x0000000088420000
    0x0000000088420000
    0x0000000088420000

Reading a set of signals
~~~~~~~~~~~~~~~~~~~~~~~~
Multiple signals may be specified by separating them with a newline.

.. code-block:: shell-session

    $ printf 'TIME board 0\nCPU_FREQUENCY_STATUS package 0\nCPU_FREQUENCY_STATUS package 1\nCPU_ENERGY package 0\nCPU_ENERGY package 1' |\
        geopmsession -n
    70.250978379,2434090909.090909,2775000000,198575.8842163086,88752.19470214844

Signals may also be specified in a separate file using the ``-i`` option.


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
   sample-time-first: "2024-08-14T18:23:58.545153099-0700"
   sample-time-total: 9.99976
   sample-count: 2001
   sample-period-mean: 0.00499988
   sample-period-std: 5.43807e-05
   metrics:
     TIME:
       count: 2001
       first: 0.0825453
       last: 10.0823
       min: 0.0825453
       max: 10.0823
       mean: 5.08268
       std: 2.88873
     CPU_POWER:
       count: 2000
       first: 71.9161
       last: 54.9997
       min: 36.8681
       max: 75.2159
       mean: 50.1323
       std: 6.61714
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 1.69773e+09
       last: 1.75341e+09
       min: 1e+09
       max: 2.49659e+09
       mean: 1.5542e+09
       std: 3.72332e+08

The same report rendered into csv format:

.. code-block:: text

    "host","sample-time-first","sample-time-total","sample-count","sample-period-mean","sample-period-std","CPU_FREQUENCY_STATUS-count","CPU_FREQUENCY_STATUS-first","CPU_FREQUENCY_STATUS-last","CPU_FREQUENCY_STATUS-min","CPU_FREQUENCY_STATUS-max","CPU_FREQUENCY_STATUS-mean","CPU_FREQUENCY_STATUS-std","CPU_POWER-count","CPU_POWER-first","CPU_POWER-last","CPU_POWER-min","CPU_POWER-max","CPU_POWER-mean","CPU_POWER-std","TIME-count","TIME-first","TIME-last","TIME-min","TIME-max","TIME-mean","TIME-std"
    "cluster-node-11","2025-03-10T21:51:23.189529258-0700",10.001955031000001,2001,0.0050009775155000005,0.00010043535451280228,2001,847115384.6153846,850000000.0,821153846.1538461,873076923.0769231,847851314.7272667,3590132.1103830505,2001,399.14601612728563,300.103422331257,274.8583842263399,399.14601612728563,321.0146130526503,17.236293555024577,2001,6.143247742,16.145202773,6.143247742,16.145202773,11.144727482043468,2.8891493308338507

.. _geopmsession.1:reading-signals-during-a-job-execution:

Reading signals during a job execution
--------------------------------------

Signals can be read and summary statistics gathered during job execution using
the ``--pid`` option. If both the ``--pid`` and ``-t`` options are used,
geopmsession will end when either the process ends or when the specified time
elapses, whichever is shorter. Below is an example gathering ``CPU_POWER``
while running ``sleep``. 


.. code-block:: shell-session

    $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" | geopmsession --pid $apppid -p 1
    [1] 862433
    nan
    223.9936557537629
    216.5137820024834
    213.0681419975341
    213.0355731062416
    212.6023058240874

An example gathering summary statistics while executing a job:

.. code-block:: shell-session

    $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" | geopmsession --pid $apppid -p 1 -r -
    [1] 863118
    "CPU_POWER-package-0"
    192.0918491664253
    229.5852100126677
    228.7564573775396
    host: "cluster-node-11"
    sample-time-first: "2025-03-11T11:44:16.347674498-0700"
    sample-time-total: 2.00134
    sample-count: 3
    sample-period-mean: 1.00067
    sample-period-std: 0.000850531
    metrics:
      CPU_POWER-package-0:
        count: 3
        first: 192.092
        last: 228.756
        min: 192.092
        max: 229.585
        mean: 216.811
        std: 21.4116


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
     sample-time-first: "2025-03-13T13:48:17.484751575-0700"
     sample-time-total: 1.001
     sample-count: 11
     sample-period-mean: 0.1001
     sample-period-std: 0.000300337
     metrics:
       CPU_POWER-package-0:
         count: 11
         first: 57.3536
         last: 158.705
         min: 57.3536
         max: 160.906
         mean: 149.869
         std: 30.7383

    ---

    host: "cluster-node-11"
    sample-time-first: "2025-03-13T13:48:18.585712016-0700"
    sample-time-total: 0.200019
    sample-count: 3
    sample-period-mean: 0.10001
    sample-period-std: 6.89358e-06
    metrics:
      CPU_POWER-package-0:
        count: 3
        first: 159.354
        last: 159.576
        min: 159.354
        max: 160.12
        mean: 159.683
        std: 0.394004
      [1]+  Done                    sleep 5

Sample csv output below shows each statistics sample output on a new row:

.. code-block:: shell-session

     $ sleep 5 & apppid=$!; echo "CPU_POWER package 0" |\
       geopmsession --pid $apppid -p 0.1 -r - -o /dev/null -s 10 -f csv
     "host","sample-time-first","sample-time-total","sample-count","sample-period-mean","sample-period-std","CPU_POWER-package-0-count","CPU_POWER-package-0-first","CPU_POWER-package-0-last","CPU_POWER-package-0-min","CPU_POWER-package-0-max","CPU_POWER-package-0-mean","CPU_POWER-package-0-std"
     "cluster-node-11","2025-03-13T13:55:06.330688844-0700",1.0008504780000003,11,0.10008504780000002,0.00022674563801204308,11,280.7375126848722,193.37615094901133,193.37615094901133,280.7375126848722,208.08585636276703,24.588834377550775
     "cluster-node-11","2025-03-13T13:55:07.431262159-0700",0.8999735640000002,10,0.09999706266666669,8.698831894196923e-05,10,191.878186704736,175.91600106253898,175.91600106253898,191.878186704736,185.4971100742497,5.352722675096535
     "cluster-node-11","2025-03-13T13:55:08.431459737-0700",0.200098412,3,0.100049206,7.587538605975476e-05,3,172.07603810585476,166.7683539071519,166.7683539071519,172.07603810585476,169.3263499495176,2.6590293883676446


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
   sample-time-first: "2024-08-14T17:50:00.895968647-0700"
   sample-time-total: 9.99973
   sample-count: 2001
   sample-period-mean: 0.00499987
   sample-period-std: 5.22108e-05
   metrics:
     TIME:
       count: 2001
       first: 0.0849912
       last: 10.0847
       min: 0.0849912
       max: 10.0847
       mean: 5.08514
       std: 2.88873
     CPU_POWER:
       count: 2000
       first: 58.2713
       last: 63.4941
       min: 41.1496
       max: 89.1348
       mean: 55.8502
       std: 6.41338
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 2.17727e+09
       last: 1.75341e+09
       min: 1e+09
       max: 2.58636e+09
       mean: 1.55674e+09
       std: 3.7101e+08

   ---

   host: "cluster-node-12"
   sample-time-first: "2024-08-14T17:50:01.033367154-0700"
   sample-time-total: 10.0003
   sample-count: 2001
   sample-period-mean: 0.00500015
   sample-period-std: 5.06152e-05
   metrics:
     TIME:
       count: 2001
       first: 0.0846359
       last: 10.0849
       min: 0.0846359
       max: 10.0849
       mean: 5.085
       std: 2.88887
     CPU_POWER:
       count: 2000
       first: 60.455
       last: 70.6912
       min: 46.394
       max: 89.6428
       mean: 61.7341
       std: 5.20186
     CPU_FREQUENCY_STATUS:
       count: 2001
       first: 1.70568e+09
       last: 1.69773e+09
       min: 1e+09
       max: 2.56136e+09
       mean: 1.54734e+09
       std: 3.63195e+08

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
