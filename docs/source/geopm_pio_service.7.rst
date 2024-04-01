geopm_pio_service(7) -- Signals and controls for the ServiceIOGroup
===================================================================

Description
-----------

The ServiceIOGroup implements the :doc:`geopm::IOGroup(3)
<geopm::IOGroup.3>` interface to provide a mechanism for
access to signals and controls provided by the GEOPM service.

This IOGroup will only provide access to the signals and controls that the
current user is allowed to access based on the configuration.  For more
information on how to configure user access, see :doc:`geopmaccess(1)
<geopmaccess.1>`.

.. note::
   Any signal or control that is available *without* using this IOGroup
   (i.e. if the current user has direct access to msr-safe or similar) will
   be preferred over this IOGroup.  If the desire is to force all users to
   route their signal and control requests through the service, access to
   the other mechanisms must be removed.

Requirements
------------

This IOGroup is not loaded by a server side PlatformIO object and is only
loaded client side if the UID is not 0 (i.e. not a privileged user).  When
this IOGroup is loaded, a session will be opened with the service enabling
save/restore of signals and controls.

This IOGroup will only load if the GEOPM service is active and will fail
otherwise.  Signals or controls that are available without using the service
(e.g. through direct access to msr-safe or similar) will be preferred over
those exposed by the service.  Care should be taken to only expose direct
access to signals or controls if the security mechanisms provided by the
ServiceIOGroup are not necessary.

Signals
-------

This IOGroup provides all signals that the user has been granted privileges to
access based on the :doc:`geopmaccess(1) <geopmaccess.1>` configuration.  The signal
names, domains, aggregation, etc. are derived directly from the IOGoup that
implements the signal.

Controls
--------

This IOGroup provides all controls that the user has been granted privileges to
access based on the :doc:`geopmaccess(1) <geopmaccess.1>` configuration.  The control
names, domains, aggregation, etc. are derived directly from the IOGoup that
implements the control.

Aliases
-------

This IOGroup provides all high-level aliases that the user has been granted privileges to
access based on the :doc:`geopmaccess(1) <geopmaccess.1>` configuration.  These aliases
are defined in the IOGroups that implement them.  This IOGroup provides no additional aliases.

See Also
--------

:doc:`geopm(7) <geopm.7>`,
:doc:`geopm_pio(7) <geopm_pio.7>`,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`,
:doc:`geopmaccess(1) <geopmaccess.1>`
