
geopmsession(1) -- Command line interface for the GEOPM service batch read features
===================================================================================

Synopsis
--------

.. code-block:: none

    geopmsession [-h] [-v] [-t TIME] [-p PERIOD] [--pid PID] [--print-header]
                 [-d DELIMITER] [--report-out REPORT_OUT]
                 [--trace-out TRACE_OUT] [--mpi-report]

Command line interface for the geopm service batch read features. This command
can be used to read signals by opening a session with the geopm service.


Read a signal
~~~~~~~~~~~~~

.. code-block:: none

    echo "SIGNAL_NAME DOMAIN DOMAIN_IDX" | geopmsession

Read a signal at a specific period for a specific timeout
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmsession -p PERIOD_IN_SECONDS -t TIMEOUT_IN_SECONDS
    geopmsession --period PERIOD_IN_SECONDS --time TIMEOUT_IN_SECONDS

Read a set of signals
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: none

    echo -e 'TIME board 0\nCPU_FREQUENCY_STATUS package 0' | geopmsession
    70.250978379,2434090909.090909

Get Help or Version
~~~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmsession -h
    geopmsession --help


Description
-----------

Command line interface for the GEOPM service read features.

This command can be used to read signals by opening a session with the
geopm service.  The user specifies which signals to read with standard
input. When no command line options are provided, a single read of the
signals requested though standard input is made and the results are
printed to the screen.

Options
-------

-h, --help  .. _help option:

    Print help message and exit

-v, --version  .. _version option:

    Print version and exit

-t TIME, --time TIME  .. _time option:

    Total run time of the session to be opened in seconds

-p PERIOD, --period PERIOD  .. _period option:

    When used with a read mode session reads all values out periodically with
    the specified period in seconds

--pid PID  .. _pid option:

    Stop the session when the given process PID ends

--omit-header  .. _header option:

    Do not print the CSV header before printing any sampled values

--delimiter .. _delimiter DELIMITER .. option:

    Delimiter used to separate values in CSV output

--report-out .. _reportout REPORT_OUT option:

    Output summary statistics into a yaml file. Note if ``--report-out=-`` is
    specified, the report will output to stdout. When used with the
    ``--enable-mpi`` option, reports from all hosts will be combined using the
    ``---`` line separator, and the output is written (stdout or to file) solely
    by the MPI process "rank 0".

--trace-out .. _traceout TRACE_OUT option:

    Output trace data into a CSV file. Note if ``--trace_out=-`` is specified,
    the trace will output to stdout which is also the default behavior. To avoid
    gathering trace data, set this parameter to ``/dev/null``.  When used with
    the ``--enable-mpi`` option, trace file names will be appended with the
    hostname combined with the ``-`` separator.  It is not possible to write the
    trace output to stdout when specifying ``--enable-mpi``, this will result in
    an error.

--enable-mpi .. _enablempi option:

    Gather reports over MPI and write to a single file. Append hostname to trace
    output file if specified (trace output to stdout not permitted). Requires
    mpi4py module.

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

.. code-block:: bash

    $ echo "MSR::THERM_STATUS# core 0" | geopmsession
    0x0000000088430800

This will execute one read of the signal.

Reading a signal periodically
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Both a polling period and timeout must be specified.
The polling period must be shorter than the timeout specified.

A 100ms polling period with a 300ms timeout is shown below:

.. code-block:: none

    echo -e 'MSR::THERM_STATUS# core 0' | geopmsession -p 0.1 -t 0.3
    0x0000000088410000
    0x0000000088420000
    0x0000000088420000
    0x0000000088420000

Reading a set of signals
~~~~~~~~~~~~~~~~~~~~~~~~
Multiple signals may be specified by separating them with a newline.

.. code-block:: none

    echo -e 'TIME board 0\nCPU_FREQUENCY_STATUS package 0\nCPU_FREQUENCY_STATUS package 1\nCPU_ENERGY package 0\nCPU_ENERGY package 1' | geopmsession
    70.250978379,2434090909.090909,2775000000,198575.8842163086,88752.19470214844

Reading a set of signals and getting summary statistics
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Summary statistics may be output to stdout by setting ``--report-out=-``.
Otherwise, the statistics will be output to the specified file path. If
unspecified, no statistics will be gathered.

.. code-block:: none

    printf 'TIME board 0\nCPU_POWER board 0\nCPU_FREQUENCY_STATUS board 0\n' | geopmsession -t 2 -p 0.01 --print-header --report-out=-

An example report is shown below:

.. code-block:: yaml

host: "123.45.6.7"
time-begin: "2024-08-09T16:39:13.378279712-0700"
time-end: "2024-08-09T16:39:23.385100904-0700"
metrics:
  TIME:
    count: 2001
    first: 0.207557
    last: 10.0613
    min: 0.207557
    max: 10.0613
    mean: 5.06262
    std: 2.8867
  CPU_POWER:
    count: 2000
    first: 87.0568
    last: 83.0846
    min: 77.2518
    max: 99.1282
    mean: 81.4213
    std: 1.86577
  CPU_FREQUENCY_STATUS:
    count: 2001
    first: 1.05875e+09
    last: 1e+09
    min: 1e+09
    max: 1.1275e+09
    mean: 1.00198e+09
    std: 1.20525e+07


Gathering Reports using MPI
---------------------------

The ``--enable-mpi`` command line option can be used to aggregate reports using
an MPI communicator.  This can be helpful when running sessions on more than one
compute node in an MPI enabled environment.  The user must install the optional
``mpi4py`` package to use the ``--enable-mpi`` command line option .  This can
be done using the OS package manager or PyPi.  When running in this way the
``geopmsession`` command line tool must be launched with a mpi launch wrapper
like ``mpiexec`` or ``mpirun``.  The user should run this command specifying one
``geopmsession`` process per compute node.  When using this option, trace output
to stdout is disabled.  The aggregated report is created by the "rank 0" process
of the geopmsession MPI communicator.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
