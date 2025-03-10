geopmbatch(1) -- Command line interface for the GEOPM batch server
==================================================================

Synopsis
--------

.. code-block:: bash

   usage: geopmbatch [--help] [--version] CLIENT_PID

Start a batch server
~~~~~~~~~~~~~~~~~~~~

.. code-block:: bash

    geopmbatch CLIENT_PID

Get Help
~~~~~~~~

.. code-block:: bash

    geopmbatch --help

Get Version
~~~~~~~~~~~

.. code-block:: bash

    geopmbatch --version


Description
-----------

Command line interface for the GEOPM batch server. The `geopmbatch` command
starts a batch server that communicates with a client process specified by the
`CLIENT_PID`. The batch server reads and writes signals and controls configured
through standard input.  The `geopmbatch` command line tool is intended for use
by `geopmd` to create batch servers, not as a tool for end users.

The `CLIENT_PID` argument specifies the process ID of the client that the batch
server will communicate with. The batch server will run until it receives a
termination signal or an error occurs.

The `geopmbatch` command reads from standard input to configure the signals and
controls that the batch server will support. Each line of input should specify a
request in the format:

.. code-block:: text

    read SIGNAL_NAME DOMAIN DOMAIN_IDX
    write CONTROL_NAME DOMAIN DOMAIN_IDX

Lines starting with the keyword "read" will configure signals to be read, and
lines that begin with the keyword "write" will configure controls to be
written. The `SIGNAL_NAME` and `CONTROL_NAME` specify the names of the signals
and controls, respectively. The `DOMAIN` specifies the domain type (e.g.,
"board", "package", "core", etc.), and `DOMAIN_IDX` specifies the domain index.

Options
-------

--help  .. _help option:

    Print help message and exit.

--version  .. _version option:

    Print version and exit.

Examples
--------

Some examples of how to use the ``geopmbatch`` command line tool are provided.

Starting a batch server
~~~~~~~~~~~~~~~~~~~~~~~
To start a batch server for a client process with PID 12345:

.. code-block:: shell-session

    $ echo -e "read CPU_FREQUENCY_STATUS board 0\nwrite CPU_FREQUENCY_MAX_CONTROL board 0" | geopmbatch 12345

This will start the batch server and it will communicate with the client process
with PID 12345. The batch server will read the signal named
"CPU_FREQUENCY_STATUS" when batch read is requested and write the control named
"CPU_FREQUENCY_MAX_CONTROL" when batch write is requested.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmsession(1) <geopmsession.1>`
