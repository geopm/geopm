
Guide for Service Administrators
================================

The ``geopmaccess`` command line tool is used by a system
administrator to manage access to the features provided by the GEOPM
Service.  The GEOPM Service does not allow read or write access for
any non-root user until the system administrator explicitly configures
the service using the ``geopmaccess`` command line tool.  This command
line interface allows the administrator to set access permissions for
all users, and may extend these default privileges for specific Unix
groups.


Example: Reading the Access Lists
---------------------------------

The signals and controls provided by the GEOPM Service depend on which
Linux features are avaiable, i.e. which device drivers are loaded, how
they are configured, and what hardware is installed.  The service may
be extended with IOGroup plugins which may augment the signals and
controls available through the service.

The ``geopmaccess`` commandline tool can be used to discover which
signals and controls are provided by the GEOPM Service on your system.

.. code-block:: bash

    # LIST ALL SIGNALS PROVIDED
    geopmaccess -a

    # LIST ALL CONTROLS PROVIDED
    geopmaccess -a -c

    # LIST DEFAULT SIGNAL ACCESS LIST
    geopmaccess

    # LIST DEFAULT CONTROL ACCESS LIST
    geopmaccess -c

    # LIST SIGNAL ACCESS FOR A UNIX GROUP: "power"
    geopmaccess -g power

    # LIST CONTROL ACCESS FOR A UNIX GROUP: "power"
    geopmaccess -c -g power


Example: Enabling User Access
-----------------------------

This example configures the GEOPM Service to enable any user to read
and write to bits 8-15 of the MSR_PERF_CTL register which controls the
maximum frequency of the core.


.. code-block:: bash

    # NAME OF CONTROL
    CONTROL=MSR::PERF_CTL:FREQ

    # CREATE TEMPORARY FILES
    ACCESS_SIGNALS=$(mktemp)
    ACCESS_CONTROLS=$(mktemp)

    # SAVE INITIAL ACCESS SETTINGS
    geopmaccess > ${ACCESS_SIGNALS}
    geopmaccess -c > ${ACCESS_CONTROLS}

    # ADD THE CONTROL INTO ACCESS LIST FOR READING AND WRITING
    echo ${CONTROL} >> ${ACCESS_SIGNALS}
    echo ${CONTROL} >> ${ACCESS_CONTROLS}
    geopmaccess -w < ${ACCESS_SIGNALS}
    geopmaccess -w -c < ${ACCESS_CONTROLS}

    # CLEAN UP TEMPORARY FILES
    rm ${ACCESS_SIGNALS} ${ACCESS_CONTROLS}


Example: Enabling Group Access
------------------------------

As an administrator you may want to enable access to a signal or
control for a subset of your users.  This can be accomplished by
creating a Unix user group containing the users that should be
provided this privilege.  This mechanism may also be used to extend
permissions for one particular user if the user-name-specific group is
provided.


.. code-block:: bash

    # NAME OF SIGNAL
    SIGNAL=INSTRUCTIONS_RETIRED

    # NAME OF UNIX GROUP
    GROUP_NAME=perf

    # CREATE A TEMPORARY FILE
    ACCESS_SIGNALS=$(mktemp)

    # SAVE INITIAL ACCESS SETTINGS
    geopmaccess -g ${GROUP_NAME} > ${ACCESS_SIGNALS}

    # ADD THE SIGNAL INTO ACCESS LIST FOR READING
    echo ${SIGNAL} >> ${ACCESS_SIGNALS}
    geopmaccess -g ${GROUP_NAME} -w < ${ACCESS_SIGNALS}

    # CLEAN UP TEMPORARY FILE
    rm ${ACCESS_SIGNALS}
