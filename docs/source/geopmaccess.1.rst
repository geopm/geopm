
geopmaccess(1) -- Access management for the GEOPM Service
=========================================================

Synopsis
--------

.. code-block:: none

    geopmaccess [-h] [-v] [-c] [-u | -g GROUP | -a | -l | -s] [-w | -e | -D] [-n | -F] [-x]

Access management for the GEOPM Service. Command line tool for reading and
writing the access management lists for the GEOPM Service signals and controls.

Read Access List
~~~~~~~~~~~~~~~~

.. code-block:: none

    geopmaccess [-c] [-u | -g GROUP | -a | -l]

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


Description
-----------

The GEOPM service uses the ``/etc/geopm`` directory to store files that control
which signals and controls may be accessed by a user through the service.  The
purpose of the ``geopmaccess`` command line tool is to enable reading and
writing of these access list files.


Options
~~~~~~~

-h, --help  .. _help option:

    Print help message and exit

-v, --version  .. _version option:

    Print version and exit

-c, --controls  .. _controls option:

    Command applies to controls not signals

-u, --default  .. _default option:

    Print the default user access list

-g GROUP, --group GROUP  .. _group option:

    Read or write the access list for a specific Unix GROUP

-a, --all  .. _all option:

    Print all signals or controls supported by the service system

-l, --log  .. _log option:

    Print list of used signals or controls used since last restart of the
    service

-s, --msr-safe  .. _msr-safe option:

    Generate an allowlist for msr-safe

-w, --write  .. _write option:

    Use standard input to write an access list. Implies -u unless -g is
    provided.

-e, --edit  .. _edit option:

    Edit an access list using EDITOR environment variable, default vi

-D, --delete  .. _delete option:

    Remove an access list for default user or a particular Unix Group

-n, --dry-run  .. _dry-run option:

    Do error checking on all user input, but do not modify configuration files

-F, --force  .. _force option:

    Write access list without validating GEOPM Service support for names

-x, --direct  .. _direct option:

    Write directly to files, do not use DBus


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

By convention, all control settings can be read by requesting the
signal that shares the same name as the control.  Note that when
adding a control name to the access list for writing, the
administrator is implicitly providing read access to the control
setting as well.

All users of the system will have access to the signals and controls
determined by the "default access list."  The default access list may
be read by specifying the ``-u`` / ``--user`` option.  These
permissions are extended for each user based on Unix user group
membership.  Each Unix group on the system may have GEOPM Service
signal and control access lists associated with it.  To read or write
the access list for a particular Unix user group, the ``-g`` or
``--group`` option must be specified.  The user may specify a Unix
user group name or a Unix user group ID number.  If the user specifies
a Unix grouip name, then the ID number will be queried from the OS.


Access Management
~~~~~~~~~~~~~~~~~

Reading access lists may be done by an unprivileged client or by the
system administrator, but only a process with the Linux
`capabilities(7) <https://man7.org/linux/man-pages/man7/capabilities.7.html>`_
``CAP_SYS_ADMIN`` has permission to write or delete an access list.
Typically is done by the root user or with the ``sudo`` command.  The
administrator may execute ``geopmaccess`` to write to an access list
by providing the ``-w`` / ``--write`` command line option.  The ``-D``
/ ``--delete`` option will remove all signals or controls from the
configuration.  An access list can be modified in a text editor when
the ``-e`` / ``--edit`` option is provided.

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

There are use cases where the ``/etc/geopm`` directory must be
configured on a system where the signals and controls available at
configuration-time do not match what is available at run-time.  This
is particularly common when the ``/etc/geopm`` directory is
located on a shared file system to support distributed servers.

The ``-n`` / ``--dry-run`` option may be specified to check the
validity of a configuration at run-time without modifying files in the
``/etc`` directory.  This option will check the names provided
to standard input, however no files are opened for writing.

The ``-F`` / ``--force`` option enables the creation of access
lists in ``/etc/geopm`` without checking that the names in the
access list correspond to signals or controls supported by the active
GEOPM Service.  This enables the creation of the configuration file on
a system where the GEOPM Service does not support some signals or
controls.

To avoid any issues due to differences in OS user group name/ID look up on
shared file systems, it may be preferred to specify the Unix user group ID
number rather than the name when using the ``-g`` / ``--group`` option.

Note that having signal or control names in an access list in
``/etc/geopm`` which are not valid on a particular system is
not an error.  This enables access list files to be mounted on
multiple systems which may have non-overlapping support.

Examples
--------

Some examples of how to use the ``geopmaccess`` command line tool are
provided.

Reading the Access Lists
~~~~~~~~~~~~~~~~~~~~~~~~

The signals and controls provided by the GEOPM Service depend on which
Linux features are available, i.e. which device drivers are loaded, how
they are configured, and what hardware is installed.  The service may
be extended with IOGroup plugins which may augment the signals and
controls available through the service.

The ``geopmaccess`` commandline tool can be used to discover which
signals and controls are provided by the GEOPM Service on your system.

.. code-block:: bash

    # PRINT ALLOWED SIGNALS FOR CALLING USER
    geopmaccess

    # PRINT ALLOWED CONTROLS FOR CALLING USER
    geopmaccess --controls

    # PRINT ALL SIGNALS PROVIDED
    geopmaccess --all

    # PRINT ALL CONTROLS PROVIDED
    geopmaccess --all --controls

    # PRINT DEFAULT SIGNAL ACCESS LIST
    geopmaccess --default

    # PRINT DEFAULT CONTROL ACCESS LIST
    geopmaccess --default --controls

    # PRINT SIGNAL ACCESS FOR UNIX GROUP NAME "power"
    geopmaccess --group power

    # PRINT CONTROL ACCESS FOR UNIX GROUP ID 101
    geopmaccess --controls --group 101


Enabling User Access
~~~~~~~~~~~~~~~~~~~~

This example configures the GEOPM Service to enable any user to read
and write to bits 8-15 of the MSR_PERF_CTL register which controls the
maximum frequency of the core.  This could also be accomplished
interactively using the ``-e`` / ``--edit`` option.


.. code-block:: bash

    # NAME OF CONTROL
    CONTROL=MSR::PERF_CTL:FREQ

    # CREATE TEMPORARY FILES
    ACCESS_SIGNALS=$(mktemp)
    ACCESS_CONTROLS=$(mktemp)

    # SAVE INITIAL ACCESS SETTINGS
    geopmaccess > ${ACCESS_SIGNALS}
    geopmaccess --controls > ${ACCESS_CONTROLS}

    # ADD THE CONTROL INTO ACCESS LIST FOR READING AND WRITING
    echo ${CONTROL} >> ${ACCESS_SIGNALS}
    echo ${CONTROL} >> ${ACCESS_CONTROLS}
    geopmaccess --write < ${ACCESS_SIGNALS}
    geopmaccess --write --controls < ${ACCESS_CONTROLS}

    # CLEAN UP TEMPORARY FILES
    rm ${ACCESS_SIGNALS} ${ACCESS_CONTROLS}


Enabling Group Access
~~~~~~~~~~~~~~~~~~~~~

As an administrator you may want to enable access to a signal or
control for a subset of your users.  This can be accomplished by
creating a Unix user group containing the users that should be
provided this privilege.  This mechanism may also be used to extend
permissions for one particular user if the user-name-specific group is
provided.  This can also be accomplished interactively using the
``-e`` / ``--edit`` option.


.. code-block:: bash

    # NAME OF SIGNAL
    SIGNAL=CPU_INSTRUCTIONS_RETIRED

    # NAME OF UNIX GROUP
    GROUP_NAME=perf

    # CREATE A TEMPORARY FILE
    ACCESS_SIGNALS=$(mktemp)

    # SAVE INITIAL ACCESS SETTINGS
    geopmaccess --group ${GROUP_NAME} > ${ACCESS_SIGNALS}

    # ADD THE SIGNAL INTO ACCESS LIST FOR READING
    echo ${SIGNAL} >> ${ACCESS_SIGNALS}
    geopmaccess --group ${GROUP_NAME} --write < ${ACCESS_SIGNALS}

    # CLEAN UP TEMPORARY FILE
    rm ${ACCESS_SIGNALS}


Another example of using group permissions is to create a Unix group
called "geopm" that is granted access to all signals and controls that
are enabled on the system.  Users that should be granted full use of
the service can then be added to the *geopm* Unix group.


.. code-block:: bash

    # NAME OF UNIX GROUP
    GROUP_NAME=geopm

    # CREATE "geopm" GROUP
    groupadd ${GROUP_NAME}

    # ENABLE GROUP TO READ ALL AVAILABLE SIGNALS
    geopmaccess -a | geopmaccess -g ${GROUP_NAME} -w

    # ENABLE GROUP TO WRITE ALL AVAILABLE CONTROLS
    geopmaccess -a -c | geopmaccess -g ${GROUP_NAME} -w -c


Supporting Heterogeneous Clusters
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This example demonstrates how to create and check access lists when
the ``/etc/geopm`` directory must be modified on a system with
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

    # Log onto node where /etc/geopm is writable
    ssh admin-system

    # Combine the created lists, duplicates are okay
    cat system1-signals.txt system2-signals.txt > all-signals.txt
    cat system1-controls.txt system2-controls.txt > all-controls.txt

    # Modify configuration without checking names
    sudo geopmaccess --write --force < all-signals.txt
    sudo geopmaccess --write --controls --force < all-controls.txt

Configuring msr-safe
~~~~~~~~~~~~~~~~~~~~

In order for the GEOPM service to be able to leverage the msr-safe
kernel driver for fast MSR access, an allowlist must be put in place.
Without the allowlist, msr-safe will not provide access to MSRs even
for privileged users.  For more information see: :ref:`requires:The MSR
Driver`.

The following example demonstrates the necessary commands to set the
msr-safe allowlist using ``geopmaccess``.

.. note::
    Note that if the ``msr`` driver is already loaded (via inspection
    with ``lsmod``) then steps 1 and 4 below can be skipped.

.. code-block:: bash

    # Run the following with root or sudo:
    1. modprobe msr
    2. geopmaccess -s > allowlist.txt
    3. cat allowlist.txt > /dev/cpu/msr_allowlist
    4. rmmod msr

If you have installed msr-safe via RPM, it is likely you have the
msr-safe.service running via systemd that will try to set the
allowlist on service start.  In order to have msr-safe.service use the
allowlist you have just created, you must create or modify the
msr-safe entry in sysconfig to set the ``ALLOWLIST`` environment
variable.

First, place ``allowlist.txt`` under the default path for allowlists
from the RPM install: ``/usr/share/msr-safe/allowlists``.  Next,
create ``/etc/sysconfig/msr-safe`` with the following contents:

.. code-block:: bash

    ALLOWLIST=<PATH_TO_ALLOWLIST_IN_USR_SHARE>

Afterward, use ``systemctl restart msr-safe`` to restart that
service and the manually verify that the allowlist was set properly.
This can be done by issuing ``cat /dev/cpu/msr-allowlist`` and
comparing the entries against ``allowlist.txt``.

More information:
`msr-safe.service
<https://github.com/LLNL/msr-safe/blob/main/rpm/msr-safe.service>`_
`msr-safe.sh
<https://github.com/LLNL/msr-safe/blob/main/rpm/msr-safe.sh>`_

Exit Status
-----------

The ``geopmaccess`` command will return 0 upon success and -1 on
failure.  For all failures, an error message describing the failure
will be printed.  Setting the ``GEOPM_DEBUG`` environment variable
will enable more verbose error messages.

Use of the ``geopmaccess`` command line tool requires the GEOPM
Service systemd unit to be active.  This can be checked with the
command ``systemctl status geopm``.  A failure will occur if the GEOPM
Service is not active.

Modification of access lists is typically is done by the root user or
with the ``sudo`` command.  A process is required to have the Linux
`capabilities(7) <https://man7.org/linux/man-pages/man7/capabilities.7.html>`_
``CAP_SYS_ADMIN`` in order to modify any access lists, and a failure
will occur otherwise.

Some command line options cannot be used together, for example, a
failure will occur if the user specifies both ``--all`` and
``--write``.

Attempts to set configurations using unsupported names will fail
unless the ``-F`` / ``--force`` option is provided.


See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopmread(1) <geopmread.1>`,
:doc:`geopmwrite(1) <geopmwrite.1>`,
:doc:`geopmsession(1) <geopmsession.1>`,
:doc:`geopm_pio(7) <geopm_pio.7>`
