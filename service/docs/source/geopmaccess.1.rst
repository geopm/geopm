
geopmaccess(1) -- Access management for the GEOPM Service
=========================================================

SYNOPSIS
--------

Read Access List
~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess [-c] [-u | -g GROUP | -a]


Write Access List
~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -w [-c] [-g GROUP] [-n | -f]

Edit Access List
~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -e [-c] [-g GROUP]


Remove Access List
~~~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess -D [-c] [-g GROUP]



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
  Command applies to controls not signals

* ``-u``, ``--default``:
  Print the default user access list

*
  ``-g``, ``--group``:
  Read or write the access list for a specific Unix GROUP

*
  ``-a``, ``--all``:
  Print all signals or controls supported by the service system

*
  ``-w``, ``--write``:
  Use standard input to write an access list

*
  ``-e``, ``--edit``:
  Edit an access list using EDITOR environment variable, default ``vi``

*
  ``-D``, ``--delete``:
  Remove an access list for default user or a particular Unix Group

*
  ``-n``, ``--dry-run``:
  Do error checking on all user input, but do not modify configuration
  files

*
  ``-F``, ``--force``:
  Write access list without validating GEOPM Service support for names

*
  ``-h``, ``--help``:
  Print brief summary of the command line usage information, then
  exit

*
  ``-v``, ``--version``:
  Print version of `geopm(7) <geopm.7.html>`_ to standard output, then
  exit



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
the configuration.  An access list can be modified in a text editor
when the ``-e`` / ``--edit`` option is provided.

When writing an access list with the ``-w`` / ``--write`` command line
option, the list of names is provided to standard input.  Typically,
this is piped in from an existing file.  When the ``-e`` / ``--edit``
option is provided, the existing access list is opened in an editor
for modification.  The default editor is ``vi`` but the user may
override this with the ``EDITOR`` environment variable.

The access list created is derived from standard input or the edited
file by reading one name per line.  Standard input lines that begin
with the ``#`` character are ignored, and an empty line or ``EOF``
*(end of file)* will cause parsing of standard input to stop.

Note that the ``-a`` / ``--all`` options are not valid when writing,
editing, or deleting an access list.  The default access list will be
written or deleted if the ``-g`` / ``--group`` option is not
specified.  This differs from the default behavior when reading an
access list: the default behavior when writing equivalent to the
``-u`` / ``--default`` option when reading.


Shared File Systems
~~~~~~~~~~~~~~~~~~~

There are use cases where the ``/etc/geopm-service`` directory must be
configured on a system where the signals and controls available at
configuration-time do not match what is available at run-time.  This
is particularly common when the ``/etc/geopm-service`` directory is
located on a shared file system to support distributed servers.

The ``-n`` / ``--dry-run`` option may be specified to check the
validity of a configuration at run-time without modifying files in the
``/etc`` directory.  This option will check the names provided
to standard input, however no files are opened for writing.

The ``-F`` / ``--force`` option enables the creation of access
lists in ``/etc/geopm-service`` without checking that the names in the
access list correspond to signals or controls supported by the active
GEOPM Service.  This enables the creation of the configuration file on
a system where the GEOPM Service does not support some signals or
controls.

Note that having signal or control names in an access list in
``/etc/geopm-service`` which are not valid on a particular system is
not an error.  This enables access list files to be mounted on
multiple systems which may have non-overlapping support.

EXAMPLES
--------

This example demonstrates how to create and check access lists when
the ``/etc/geopm-service`` directory must be modified on a system with
incomplete support for signals and controls.

In this example, the access lists created contain all signals and
controls supported by two different systems.  Similar steps would be
followed if the input lists were derived in a different way.  This
example also shows how to validate access lists on multiple systems
and combine access lists when writing to a shared mount point.


.. code-block :: bash

    # Log onto one of the systems
    ssh system1

    # Create a lists of signals and controls on shared mount
    geopmaccess --all > system1-signals.txt
    geopmaccess --all --controls > system1-controls.txt

    # Check validity of created lists
    geopmaccess --write --dry-run < system1-signals.txt
    geopmaccess --write --controls --dry-run < system1-controls.txt

    # Log onto a system with non-overlapping support
    ssh system2

    # Create lists of signals and controls on shared mount
    geopmaccess --all > system2-signals.txt
    geopmaccess --all --controls > system2-controls.txt

    # Check validity of created lists
    geopmaccess --write --dry-run < system2-signals.txt
    geopmaccess --write --controls --dry-run < system2-controls.txt

    # Log onto node where /etc/geopm-service is writable
    ssh admin-system

    # Combine the created lists, duplicates are okay
    cat system1-signals.txt system2-signals.txt > all-signals.txt
    cat system1-controls.txt system2-controls.txt > all-controls.txt

    # Modify configuration without checking names
    sudo geopmaccess --write --force < all-signals.txt
    sudo geopmaccess --write --controls --force < all-controls.txt


EXIT STATUS
-----------

*TODO*

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopmread(1) <geopmread.1.html>`_\ ,
`geopmwrite(1) <geopmwrite.1.html>`_\ ,
`geopmsession(1) <geopmsession.1.html>`_\ ,
`SKX Platform Controls <controls_SKX.html>`_\ ,
`SKX Platform Signals <signals_SKX.html>`_
