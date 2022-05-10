
geopmaccess(1) -- Access management for the GEOPM Service
=========================================================

SYNOPSIS
--------


Read Access List
~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess [-c] [-g GROUP | -a]


Write Access List
~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -w [-c] [-g GROUP] [-n | -s]


Remove Access List
~~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -D [-c] [-g GROUP] [-n]



Get Help or Version
~~~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -h | -v


DESCRIPTION
-----------

The GEOPM service uses the ``/etc/geopm-service`` directory to store
files that control which signals and controls may be accessed by a
user through the service.  The purpose of the ``geopmaccess`` command
line tool is to enable reading and writing of these access list files.


Options
~~~~~~~

*
  ``-c``, ``--controls``:
  Command applies to controls not signals.


* ``-u``, ``--user``:
  Read the default user access list.

*
  ``-g``, ``--group``:
  Read or write the access list for a specific Unix GROUP.

*
  ``-a``, ``--all``:
  Print all signals or controls supported by the service system (not
  valid with ``-w`` option).

*
  ``-w``, ``--write``:
  Write default access list, or the access list for a particular Unix
  group.

*
  ``-D``, ``--delete``:
  Remove an access list for default user or a particular Unix Group.
*
  ``-n``, ``--dry-run``:
  Do error checking on all user input, but do not modify configuration
  files

*
  ``-E``, ``--skip-check``:
  Write access list to disk without error checking.

*
  ``-h``, ``--help``:
  Print brief summary of the command line usage information,
  then exit.

*
  ``-v``, ``--version``:
  Print version of `geopm(7) <geopm.7.html>`_ to standard output, then exit.



Query Access
~~~~~~~~~~~~

The default behavior of ``geopmaccess`` when no options are provided
is to print the list of signals that the calling user has permissions
to read through the GEOPM Service. This command is used by a GEOPM
service client to discover which signals they have permission to read
based on the configuration set by the system administrator.  All
supported signals or controls are printed when called by a root user.

All ``geopmaccess`` commands refer to reading or writing signal access
lists by default.  To specify that the command should instead refer to
the access lists for controls, the ``-c`` / ``--controls`` option must
be provided.  For example, when the ``-c`` option is provided without
any other options, the list of controls that may be configured by the
calling user is printed.

All users of the system will have access to the signals and controls
determined by the "default access list."  The default access list may
be read by specifying the ``-u`` / ``--user`` option.  These
permissions are extended for each user based on Unix user group
membership.  Each Unix group on the system may have GEOPM Service
signal and control access lists associated with it.  To read or write
the access list for a particular Unix user group, the ``-g`` or
``--group`` option must be specified.


Access Management
~~~~~~~~~~~~~~~~~

Reading access lists may be done by an unprivileged client or by the
system administrator, but only a process with the Linux capability
``CAP_SYS_ADMIN`` has permission to write or delete an access list.
The administrator may execute ``geopmaccess`` to write to an access
list by providing the ``-w`` / ``--write`` command line option.  The
``-D`` / ``--delete`` option will remove all signals or controls from
the configuration.

The access list created is derived from standard input by reading one
name per line.  Standard input lines that begin with the ``#``
character are ignored, and an empty line or EOF (end of file) will
cause parsing of standard input to stop.

Note that the ``-a`` / ``--all`` options are not valid when writing or
deleting an access list.  The default access list will be written or
deleted if the ``-g`` / ``--group`` option is not specified.


Shared File Systems
~~~~~~~~~~~~~~~~~~~

There are use cases where the ``/etc/geopm-service`` directory must
be configured on a system where the GEOPM service is not active, or
where the signals and controls available at configuration-time do not
match what is available at run-time.  This is particularly common when
the ``/etc/geopm-service`` directory is located on a shared file
system to support distributed servers.

The ``-n`` / ``--dry-run`` option may be specified to check the
validity of a configuration at run-time without modifying files in the
``/etc`` file system.  This option will check the names provided to
standard input, and will verify any Unix group name specified.  No
configuration files are modified when this option is specified.

The ``-E`` / ``--skip-check`` option enables the creation of access
lists in ``/etc/geopm-service`` without checking that the names in the
access list correspond to signals or controls supported by the active
GEOPM Service.  This enables the creation of the configuration file on
a system where the GEOPM Service does not support some signals or
controls.

Note that having signal or control names in an access list in
``/etc/geopm-service`` which are not valid on a particular system is
not an error.  This enables access list files to be mounted on
multiple systems which may have non-overlapping support.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmsession(1) <geopmsession.1.html>`_\ ,
`SKX Platform Controls <controls_SKX.html>`_\ ,
`SKX Platform Signals <signals_SKX.html>`_
