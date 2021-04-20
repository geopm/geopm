GEOPM systemd Service and Daemon
--------------------------------

The GEOPM service enables a client to make measurements from the
hardware platform and set hardware control parameters.  Fine grained
access management is configurable by system administrators with the
`geopmaccess` command line tool.

The `geopmd` daemon is started by the GEOPM systemd service and uses
D-Bus for communication with client processes and for administrator
configuration.  The geopm service is used to extend the
geopm::PlatformIO and geopm::PlatformTopo features.  The PlatformIO
interface of GEOPM is extended with IOGroups.  The geopm service
enables the ServiceIOGroup which provides the IOGroup features through
interfacing with geopmd over the D-Bus interface.  The ServiceIOGroup
is loaded first by PlatformIO for any non-root process, and is not
loaded for any process owned by root.  Any signals or controls that
can be provided by native IOGroups will be used because they are
loaded after the ServiceIOGroup.  The ServiceIOGroup will only be used
when a user requests signals or controls that cannot be provided by
any of the IOGroups loaded by the unprivileged user process.

The geopm service also provides a fail-safe save/restore mechanism for
any platform controls that are exposed by PlatformIO.  This is done by
initiating a session with the service when PlatformIO is first created
by an unprivileged process.  All controls are saved by the GEOPM
service prior to opening the session with the end-user.  When the
session is closed, either by an explicit D-Bus call by the client, or
when the process that initiated the client session ends, all control
knobs are reset to the values that they had prior to opening the
session.  Some filtering may be applied to the raw signals provided by
the hardware interface before being exposed to the client session.  In
particular, all monotonic signals (e.g. hardware counters) are
reported with respect to the value they had when the session began,
which is reported as zero.

A fast batch mechanism is available to support the
ServiceIOGroup::read_batch() and ServiceIOGroup::write_batch()
interfaces.  A client process using the ServiceIOGroup for batch
operations opens a batch session through the D-Bus interface and
requests access to a desired set of signals and/or controls.  Once
this session is established after validating all requested signals and
controls a protocol between the server and client is enabled that uses
inter-process shared memory and Unix signals for fast access rather
than the slow D-Bus interface.

Signals and Controls
--------------------

Each GEOPM signal and control has an associated name and a hardware
domain.  The name is a unique string identifier defined by the IOGroup
that provides the signal or control.  The hardware domain that
provides the interface is represented by one of the enumerations as
defined by PlatformTopo.  A detailed description of the signals and
controls available on a system can be discovered with the
`geopmaccess` command line tool.  The description includes information
useful for end-users, and it also provides information for system
administrators that will help them understand what is enabled for an
end-user when access is granted to a signal or control.  The platform
topology information about a system can also be shown through
`geopmaccess`.


Access Control
--------------

Access to signals and controls is determined by Unix group
associations.  A default set of access is configured for all users on
the system and this list can be augmented by lists associated with
Unix groups.  A user will have access to all of the signals and
controls provided in the default access list as well as any access
lists that are associated with groups that they belong to.  The
default lists are stored in:

    /etc/geopm-service/0.DEFAULT_ACCESS/allowed_signals
    /etc/geopm-service/0.DEFAULT_ACCESS/allowed_controls

Each Unix `<GROUP>` that has extended permissions can maintain one or
both of the files

    /etc/geopm-service/<GROUP>/allowed_signals
    /etc/geopm-service/<GROUP>/allowed_controls

Any missing files are inferred to be empty lists, including the
default access files.  A signal or control will not be available to
non-root users until a system administrator enables access through
these allow lists.  It is recommended that all manipulation of these
files should be done through the GEOPM service with the geopmaccess
command line tool.


Opening a Session
-----------------

A client process opens a session with the GEOPM service in order to
get access to hardware signals and controls.  The request to open a
session is done through the io.github.geopm.PlatformOpenSession D-Bus
interface which has bindings accessible from C++, python and the
geopmaccess command line tool.  The GEOPM controller will open a
session if it detects that the geopm systemd service is active.  The
PlatformIO::platform_io() factory method will always return a
PlatformIO implementation that is derived from the service if the
service is active and the user is not the root user.


The io.github.geopmPlatformOpenSession API has the following D-Bus
interface definition:

            <method name="PlatformOpenSession">
                <arg direction="in" name="signal_config" type="as" />
                <arg direction="in" name="control_config" type="as" />
                <arg direction="in" name="interval" type="d" />
                <arg direction="in" name="protocol" type="i" />
                <arg direction="out" name="session" type="(ixxs)" />



by specifying a
list of the signals and controls that will be exposed by the daemon.
Each signals and control requests is provided by a structure with a
name string, a domain enum and the index of the domain.  An example
request within the list of requested controls might be

    ("PACKAGE_POWER_LIMIT", GEOPM_DOMAIN_PACKAGE, 1)

which would specify the power limit for package number 1.  Similarly
the number of instructions retired for core 42 would be specified as

    ("INSTRUCTIONS_RETIRED", GEOPM_DOMAIN_CORE, 42)

in the list of signals.  Note the domain enumerations are provided in
the "geopm_topo.h" header file.  The user also specifies an interval
in seconds.  This interval controls how frequently


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
