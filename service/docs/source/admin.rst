
Guide for Service Administrators
================================

This documentation covers some of the aspects of the GEOPM Service
that are important to system administrators.  These include how the
GEOPM Service is integrated with the Linux OS, which directories are
created and modified by the GEOPM Service, how the files in those
directories are used, and a command line tool to configure the GEOPM
Service.  Additional information is available on other pages:

- :doc:`Install Guide <install>`
- :doc:`Security Guide <security>`


Linux Integration
-----------------

The GEOPM Service integrates with the Linux OS through Systemd as a
unit that is installed with the geopm-service RPM.  The ``sytemctl``
command can be used to interact with the ``geopm`` Systemd Unit.


GEOPM Service Files
-------------------

In addition to the files provided by the installation packages, the
GEOPM Service may create and modify files while active.  The files
created by the GEOPM Service are located within two directories.  The
files in ``/etc/geopm`` hold the access control lists.  These
files persist across reboots and restarts of the service.  The files
in ``/run/geopm`` track information about clients that are
actively using the service.  These files save the state of the GEOPM
Service and if the service is stopped for any reason the files will be
used when the service is started again.  The files in ``/run`` are
erased upon reboot.

All files and directories within ``/etc/geopm`` or
``/run/geopm`` are created by the GEOPM Service with
restricted access permissions and root ownership.  The GEOPM Service
will not read any file or directory if they are modified to have more
permissive access restrictions, non-root ownership, or if they are
replaced by a symbolic link or other non-regular file.  If these
checks fail, the file or directory will be renamed to include a UUID
string and a warning is printed in the syslog.  These renamed files or
directories enable an administrator to perform an investigation into
problem, but they will not be used by the GEOPM Service in any way.

It is recommended that these GEOPM Service system files are always
manipulated using GEOPM tools like ``geopmaccess``, however, any
administrator that manipulates the GEOPM system files without using a
GEOPM interface should be aware of the permission and ownership
requirements for these files.  For more information about the GEOPM
security model please refer to the `Security Guide <security.html>`_.


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
