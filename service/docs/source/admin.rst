
Guide for Service Administrators
================================

This documentation covers some of the aspects of the GEOPM Service
that are important to system administrators.  These include how the
GEOPM Service is integrated with the Linux OS, which directories are
created and modified by the GEOPM Service, how the files in those
directories are used, and a command line tool to configure the GEOPM
Service.  Additional information is available on other pages:

- `Install Guide <install.html>`_
- `Security Guide <security.html>`_


Linux Integration
-----------------

The GEOPM service integrates with the Linux OS through Systemd as a
unit that is installed with the geopm-service RPM.  The ``sytemctl``
command can be used to interact with the ``geopm`` Systemd Unit.


GEOPM Service Files
-------------------

In addition to the files installed and maintained by the installation
packages, the GEOPM service may create and modify files at runtime.
These include files in ``/etc/geopm-service`` that control access
settings and files in ``/run/geopm-service`` that track information
about clients that are actively using the service.

Configuring Access Lists
------------------------

The `geopmaccess(1) <geopmaccess.1.html>`_ command line tool is used
by a system administrator to manage access to the features provided by
the GEOPM Service.  The GEOPM Service does not allow read or write
access for any non-root user until the system administrator explicitly
configures the service using the ``geopmaccess`` command line tool.
This command line interface allows the administrator to set access
permissions for all users, and may extend these default privileges for
specific Unix groups.

