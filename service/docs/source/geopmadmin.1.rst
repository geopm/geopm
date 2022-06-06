geopmadmin(1) -- tool for GEOPM system administrators
=====================================================

Synopsis
--------

``geopmadmin`` [\ ``--config-default``\ \|\ ``--config-override``\ \|\ ``--msr-allowlist``\ ] [\ ``--cpuid``\ ]

``geopmadmin`` [\ ``--help``\ \|\ ``--version``\ ]

Description
-----------

The ``geopmadmin`` command line tool facilitates the administration of
systems that are using GEOPM.  It can be used to display the path to
the GEOPM configuration files, or check the validity the contents of
those files.  This tool can also be used to configure the ``msr-safe``
kernel driver to enable the white listing of all MSR access required
by GEOPM.  When run with no arguments ``geopmadmin`` will parse the
default and override configuration files on the system and print out
in a human readable format the values determined by these files.

Options
-------
--help                 Print brief summary of the command line usage
                       information, then exit.
--version              Print version of :doc:`geopm(7) <geopm.7>` to standard
                       output, then exit.
-d, --config-default   Print the path to the GEOPM configuration file that
                       controls the default values for the system.
-o, --config-override  Print the path to the GEOPM configuration file that
                       controls the override values for the system.
-a, --msr-allowlist    Print the minimum required allowlist for the msr-safe
                       driver to enable all of the GEOPM features.
-c, --cpuid            Specify the cpuid in hexidecimal to select the
                       architecture for the msr-safe allowlist generation. If
                       this option is not specified the architecture where the
                       application is running will be used.

Examples
--------

Set the msr-safe allowlist to enable GEOPM:

.. code-block::

   geopmadmin --allowlist > /dev/cpu/msr_allowlist


on a legacy msr-safe system:

.. code-block::

   geopmadmin --allowlist > /dev/cpu/msr_whitelist


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopmagent(1) <geopmagent.1>`
:doc:`geopmlaunch(1) <geopmlaunch.1>`
