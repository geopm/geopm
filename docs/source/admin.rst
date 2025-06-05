Admin Guide
===========

This guide covers GEOPM's integration with the Linux OS, directories influenced
by GEOPM, the utilization of files within those directories, and a command-line
tool for configuring the GEOPM Service. For further details, explore the
subsequent sections:

- :doc:`Install Guide <install>`
- :doc:`Security Guide <security>`


Linux Integration
-----------------

The GEOPM Service integrates seamlessly with the Linux OS through Systemd. It is
packaged within the ``geopmd`` binary package, and administrators can install it
using their respective package management systems. Use ``systemctl`` to interact
with the ``geopm`` Systemd Unit.

**Systemctl Command Behavior**

- **start**:

  Starts the GEOPM Access Service. All required state files in `/run/geopm` are
  initialized, and the service begins accepting client connections. Any stale
  session files from previous runs are cleaned up as part of startup.

- **stop**:

  Stops the GEOPM Access Service. All active client sessions are closed
  gracefully before the service terminates. This includes restoring any hardware
  controls (such as power limits) to their default or saved state, and cleaning
  up session files in `/run/geopm`. The write-mode session (if any) is closed
  first to ensure hardware state is restored.

- **restart**:

  Re-initialize the service.  Similar to running ``stop`` followed by ``start``,
  except that all sessions remain open through the process and controls are not
  restored.

- **enable**:

  Configures the GEOPM Access Service to start automatically at boot. This does
  not immediately start the service, but ensures it will be started on the next
  system boot.

**Note:**
The service is robust to unexpected shutdowns. If the geopmd process ends for
any reason, all state required to restart is preserved.  To be certain that all
controls are restored to their original configuration, the administrator should
run ``systemctl stop geopm``. Administrators can monitor the system journal for
info and warning messages about session closure and hardware state restoration
during these operations.


GEOPM Service Files
-------------------

Beyond the files that come with the installation packages, the GEOPM Service
may generate and modify additional files during its active state. These files
are housed within two primary directories:

- ``/etc/geopm``: This directory contains configuration files, including access
  control lists. Files here persist across both reboots and service restarts.

- ``/run/geopm``: This directory contains files that monitor data about clients
  actively engaging the service, files that help maintain the GEOPM Service's
  state, and files that are used by GEOPM's save/restore mechanism. Should the
  service halt unexpectedly, these files aid in its subsequent restart. However,
  remember that the ``/run`` directory's contents get deleted upon a system reboot.

Furthermore, the GEOPM Service ensures robust security measures:

- Both ``/etc/geopm`` and ``/run/geopm`` directories and their contained files
  are established with restricted access permissions and root ownership.

- The service will avoid reading any file or directory if there's a relaxation
  in access restrictions, non-root ownership, or if they're substituted by
  symbolic links or non-standard files. Should these conditions not be met, the
  affected file or directory will be renamed with a UUID and a warning will be
  dispatched to the syslog. While these renamed entities can assist an
  administrator in investigations, they are otherwise ignored by the GEOPM Service.

For seamless operation and security, it's advised to manage the GEOPM Service
system files using GEOPM tools like ``geopmaccess``. However, administrators
opting to handle GEOPM system files outside of a GEOPM interface should be
vigilant of the necessary permission and ownership criteria. Delve deeper into
the GEOPM security intricacies by referring to the `Security Guide <security.html>`_.


Configuring Access Lists
------------------------

The :doc:`geopmaccess(1) <geopmaccess.1>` command line tool is used
by a system administrator to manage access to the features provided by
the GEOPM Service.  The GEOPM Service does not allow read or write
access for any non-root user until the system administrator explicitly
configures the service using the ``geopmaccess`` command line tool.
This command line interface allows the administrator to set access
permissions for all users, and may extend these default privileges for
specific Unix groups.


Configuring Systemd Unit File
-----------------------------

The GEOPM Systemd unit is configured with the ``geopm.service`` file that is
installed as part of the ``geopmd`` package.  This configuration file may
be amended using the command ``systemctl edit geopm.service``. See
`systemctl(1) <https://man7.org/linux/man-pages/man1/systemctl.1.html>`_ for
more details.

An administrator may wish to modify the ``GEOPM_VERBOSITY`` environment variable
set in the configuration file.  Increasing this will cause more messages to be
printed in the system journal which may assist in debugging problems where
expected signals or controls are not available.

- ``GEOPM_VERBOSITY=0``: Print errors and critical warning messages
- ``GEOPM_VERBOSITY=1``: Print warning messages
- ``GEOPM_VERBOSITY=2``: Print diagnostic info messages

The scope of messages printed when ``GEOPM_VERBOSITY`` is non-zero may increase
in the future.


The MSR Driver
--------------

Access to MSRs enhances the capabilities of the GEOPM Access Service by
providing additional hardware telemetry and controls. While the **GEOPM Access
Service** can function without access to MSRs, it provides a limited set of CPU
features. For the **GEOPM Runtime** to function correctly, these MSR-related
CPU features are necessary. Hence, MSR support is a hard requirement for
the GEOPM Runtime which may be relaxed in a future release.

One of two drivers may be used by the GEOPM Access Service to enable the MSR
features: the standard Linux (in-tree) MSR driver or the msr-safe kernel driver
maintained by LLNL.  The msr-safe driver is preferred by GEOPM if both kernel
modules are loaded because it provides low latency interface for reading and
writing many MSR values at once through an `ioctl(2)
<https://man7.org/linux/man-pages/man2/ioctl.2.html>`_ system call, possibly
improving the performance of GEOPM Runtime or other MSR usages.

The msr-safe kernel driver source code can be found `here
<https://github.com/LLNL/msr-safe>`__.  It's distributed with the `OpenSUSE
Hardware Repository <https://download.opensuse.org/repositories/hardware/>`_ and
can be installed from the RPMs provided there.  For more information about the
necessary configuration of msr-safe see: :ref:`geopmaccess.1:Configuring
msr-safe` and :ref:`the install guide <enable-service>`.  Note that subsequent to
v1.7.0 of msr-safe, it is required that the msr-safe allow list be configured
prior to starting the GEOPM Access Service.

In the absence of the msr-safe kernel driver, users may access MSRs using the
standard Linux MSR driver. This can be loaded with the command:

.. code-block:: bash

    modprobe msr

The standard MSR driver be loaded to enable MSR access through the GEOPM Systemd
Service when msr-safe is not installed.
