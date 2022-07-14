
geopmsession(1) -- Command line interface for the GEOPM service batch read features
===================================================================================

Synopsis
--------

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
~~~~~~~
-h, --help                  show this help message and exit
-t TIME, --time TIME        Total run time of the session to be opened in seconds
-p PERIOD, --period PERIOD  When used with a read mode session reads all values
                            out periodically with the specified period in seconds

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


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
