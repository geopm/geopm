GEOPM systemd Service and Daemon
--------------------------------

The GEOPM service enables a client to make measurements from the
hardware platform and set hardware control parameters.  Fine grained
permissions management for both measurements (signals) and controls is
configurable by system administrators with the `geopmaccess` command
line tool.

The `geopmd` daemon is started by the GEOPM systemd service and uses
D-Bus for communication with client processes and for administrator
configuration.  The GEOPM service is used to extend the
`geopm::PlatformIO` and `geopm::PlatformTopo` features.  The PlatformIO
interface of GEOPM is extended with IOGroups.  The GEOPM service
enables the ServiceIOGroup which provides the IOGroup features through
interfacing with `geopmd` over the D-Bus interface.  The ServiceIOGroup
is loaded first by PlatformIO in libgeopm and libgeopmpolicy, and is
not loaded by libgeopmd.  For a libgeopm or libgeopmpolicy user, any
signals or controls that can be provided by native IOGroups will be
used because they are loaded after the ServiceIOGroup.  The
ServiceIOGroup will only be used when a user requests signals or
controls that cannot be provided by any of the IOGroups loaded by the
unprivileged user process.

The GEOPM service also provides a fail-safe save/restore mechanism for
any platform controls that are exposed by PlatformIO.  This is done by
initiating a session with the service when the ServiceIOGroup is first
created by a libgeopm or libgeopmpolicy user.  All the initial values
for all controls are saved by the GEOPM service prior to opening the
session with the end-user.  When the session is closed, either by an
explicit D-Bus call by the client, or when the process that initiated
the client session ends, all control knobs are restored to the values
that they had prior to opening the session.  Some filtering may be
applied to the raw signals provided by the hardware interface before
being exposed to the client session.  In particular, all monotonic
signals (e.g. hardware counters) are reported with respect to the
value they had when the session began, which is reported as zero.

When a client calls `read_signal()` or `write_control()` on their
PlatformIO object and the only IOGroup that provides the signal or
control is the ServiceIOGroup, then each request goes through the slow
D-Bus interface.  A fast batch mechanism is available to support the
`ServiceIOGroup::read_batch()` and `ServiceIOGroup::write_batch()`
interfaces.  When a client process uses the ServiceIOGroup for batch
operations a batch session through the D-Bus interface in opened with
a request for access to a set of signals and/or controls.  Once this
session is established all requested signals and controls are validated.
Next, a protocol between the server and client is enabled that
uses inter-process shared memory and leverages POSIX signals for fast
access.  In this documentation we will call these "POSIX signals" to
differentiate from the GEOPM signal concept which is unrelated to the
POSIX signal as defined in the signal(7) man page.


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
end-user when access is granted to a signal or control.


Access Managment
----------------

Access to signals and controls through the GEOPM service is configured
by the system administrator.  The administrator controls a default
access list that applies to all users of the system.  This list can be
augmented based on Unix group associations.  The default lists are
stored in:

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
files should be done through the GEOPM service with the `geopmaccess`
command line tool.


Opening a Session
-----------------

A client process opens a session with the GEOPM service each time a
PlatformIO object is created with libgeopm or libgeopmpolicy while the
GEOPM systemd service is active.  This session is initially opened in
read-only mode.  Calls into the D-Bus APIs that modify control values:

    io.github.geopm.PlatformWriteControl
    io.github.geopm.PlatformPushControl

convert the session into read/write mode.  Only one read/write mode
session is allowed at any time.  The request will fail if a client
attempts to begin a read/write session while another client has one
open.

When a read/write mode session begins, all controls that the
service is configured to support are recorded to a save directory in:

    /var/run/geopm-service/SAVE_FILES

When a read/write mode session ends, all of these saved controls are
restored to the value they had when the session was started,
regardless of whether or not they were adjusted during the session
through the service.

The request to open a session is done in the ServiceIOGroup
constructor, and the request to close the session is made by the
ServiceIOGroup destructor.  Calls to the ServiceIOGroup's
`write_control()` or `push_control()` methods will trigger the
conversion of the session to read/write mode.  Calls to these methods
will only occur when the ServiceIOGroup is the only loaded IOGroup
that provides the control requested by the user since all IOGroups are
loaded by the PlatformIO factory after the ServiceIOGroup.

Note that if any control adjustments are made during a session through
the GEOPM service then every control supported by GEOPM will be
reverted when the session ends.  One consequence of this is that when
a control is exposed to a user only through the geopm service, then
the geopmwrite command line tool will not be effective (the value will
be written, but reverted when the geopmwrite process ends).  The
geopmaccess command line tool can be used to write any number of the
GEOPM supported controls and keep a session open for a specified
duration (or until the geopmaccess process is killed).


Starting a Batch Server
-----------------------

Outside of start-up or shutdown activities, the GEOPM runtime uses
PlatformIO through the batch interfaces to achieve higher performance
than through the `read_signal()` / `write_control()` methods.  High
performance of the batch interfaces used to gather data and enforce
policy is required for GEOPM's runtime to be effective.  The D-Bus
interface is not a high performance interface, but does enable robust
access management.  We use the D-Bus interface to fork a process that
can service the `read_batch()` and `write_batch()` implementations of the
ServiceIOGroup and use the access management features of D-Bus to
regulate which signals and controls can be pushed onto the batch
stack.

The forked process created by the `geopmd` daemon is the batch server
which reacts to the ServiceIOGroup sending POSIX signals.  To
implement the `read_batch()` method, the ServiceIOGroup sends a POSIX
signal to notify the batch server that it would like the configured
GEOPM signals to be updated in shared memory.  The batch server
reads all GEOPM signals that are being supported by the client's
ServiceIOGroup using the batch server's instance of the PlatformIO
object.  GEOPM signals are copied into the shared memory buffer and a
SIGCONT POSIX signal is sent from the batch server to the client
process when the buffer is ready.  To implement the `write_batch()`
method, the client process's ServiceIOGroup prepares the shared memory
buffer with all control settings that it is supporting.  The client
sends a SIGCONT POSIX signal to the batch server to notify it to
write the settings.  The batch server then reads the clients
settings from a shared memory buffer and writes the values through the
server process's PlatformIO instance.
