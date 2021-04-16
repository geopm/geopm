GEOPM systemd Service and Daemon
--------------------------------

The geopm service enables a client to make measurements from the
hardware platform and set hardware control parameters.  Fine grained
access management is configurable by system administrators with the
geopmaccess command line tool.

The geopmd daemon is started by the geopm systemd service and uses
D-Bus for communication with client processes and for administrator
configuration.  The geopm service provides users access to the
PlatformIO and PlatformTopo features of geopm.  This allows users to
read "signals" an write "controls" to and from hardware on the system.

A client process opens a session through a D-Bus interface, and any
controls enabled through the session are saved by the geopm service
prior to opening the session with the end user.  When the session is
closed, either by an explicit D-Bus call by the client, or when the
process that initiated the client session ends, all control knobs that
were released to the client are reset to the values that they had
prior to opening the session.  Some filtering may be applied to the
raw signals provided by the hardware interface before being exposed to
the client session.  In particular, all monotonic signals
(e.g. hardware counters) are reported with respect to the value they
had when the session began which is reported as zero.


Signals and Controls
--------------------

Each geopm signal and control has an associated name and a hardware
domain.  The name is a unique string identifier defined by the IOGroup
that provides the signal or control.  The hardware domain that
provides the interface is represented by one of the enumerations as
defined by PlatformTopo.  A detailed description of the signals and
controls available on a system can be discovered with the geopmaccess
command line tool.  The description includes information useful for
end users, and it also provides information for system administrators
that will help them understand what is enabled for an end user when
access is granted to a signal or control.  The platform topology
information about a system can also be shown through geopmaccess.


Access Control
--------------

Access to signals and controls is determined by Unix group
associations.  A default set of access is configured for all users on
the system and this list can be augmented by lists associated with
Unix groups.  A user will have access to all of the signals and
controls provided in the default access list as well as any access
lists that are associated with groups that they belong to.  The default
lists are stored in:

    /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
    /etc/geopm-service/0.DEFAULT_ACCESS/allowed_controls

Each Unix <GROUP> that has extended permissions can maintain one or both of
the files

    /etc/geopm-service/<GROUP>/allowed_signals
    /etc/geopm-service/<GROUP>/allowed_controls

Any missing files are inferred to be empty lists, including the
default access files.  A signal or control will not be available to
non-root users until a system administrator enables access through
these allow lists.  It is recommended that all manipulation of these
files should be done through the geopm service with the geopmaccess
command line tool.


Opening a Session
-----------------


POSIX Signal Control
--------------------


Session Save and Restore
------------------------


D-Bus APIs
----------

- io.github.geopm.TopoGetCache
- io.github.geopm.PlatformGetGroupAccess
- io.github.geopm.PlatformSetGroupAccess
- io.github.geopm.PlatformGetUserAccess
- io.github.geopm.PlatformGetAllAccess
- io.github.geopm.PlatformGetSignalInfo
- io.github.geopm.PlatformGetControlInfo
- io.github.geopm.PlatformOpenSession
- io.github.geopm.PlatformCloseSession
