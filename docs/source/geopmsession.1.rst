
geopmsession(1) -- Command line interface for the GEOPM service batch read features
===================================================================================

Synopsis
--------

.. code-block:: bash

   usage: geopmsession [-h] [-v] [-t TIME] [-p PERIOD] [--pid PID]
                       [--print-header | -n] [-d DELIMITER] [-r REPORT_OUT]
                       [-o TRACE_OUT] [--enable-mpi] [-f REPORT_FORMAT]
                       [-s REPORT_SAMPLES]

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
Provide the "``*``" character as the third string to request all domains
available on the system.

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
    the specified period in seconds.

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

Examples
--------

Some examples of how to use the ``geopmsession`` command line tool are
provided.

Reading a signal
~~~~~~~~~~~~~~~~
The input to the command line tool has one request per line.  A
request for reading is made of up three strings separated by white
space.  The first string is the signal name, the second string is the
domain name, and the third string is the domain index.  An example
where the entire ``THERM_STATUS`` model specific register is read from
core zero:

.. code-block:: shell-session

    $ echo "MSR::THERM_STATUS# core 0" | geopmsession -n
    0x0000000088430800

This will execute one read of the signal.

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

Reading a set of signals and getting summary statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Summary statistics may be output to stdout by setting ``--report-out=-``.
Otherwise, the statistics will be output to the specified file path. If
unspecified, no statistics will be gathered.

.. code-block:: shell-session

    $ printf 'TIME board 0\nCPU_POWER board 0\nCPU_FREQUENCY_STATUS board 0\n' |\
        geopmsession -t 10 -p 0.005 --report-out=- --trace-out=/dev/null

An example report is shown below:

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
