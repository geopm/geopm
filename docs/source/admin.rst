Service Administrators
======================

This guide covers GEOPM's integration with the Linux OS, directories
influenced by GEOPM, the utilization of files within those directories, and a
command-line tool for configuring the GEOPM Service. For further details,
explore the subsequent sections:

- :doc:`Install Guide <install>`
- :doc:`Security Guide <security>`


Linux Integration
-----------------

The GEOPM Service integrates seamlessly with the Linux OS through Systemd. It
is packaged within the geopm-service binary package, and administrators can install it
using their respective package management systems. Use ``systemctl``
to interact with ``geopm`` Systemd Unit.


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
