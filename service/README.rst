
GEOPM Service
=============

Features
--------

|:penguin:| Linux Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Linux Systemd Service with DBus interface for user-level access to
  hardware features on heterogeneous systems


|:microscope:| Hardware Telemetry
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Users read telemetry from hardware components on heterogeneous
  systems with a vendor agnostic interface


|:gear:| Hardware Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Users configure hardware component device settings on heterogeneous
  systems with a vendor agnostic interface


|:medal_sports:| Quality of Service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  GEOPM Service reverts any changes made to hardware configurations
  after the client's Linux process session ends


|:lock:| Security
~~~~~~~~~~~~~~~~~
  Linux system administrators manage fine-grained access permissions
  for capabilities exposed by the GEOPM Service


|:rocket:| Performance
~~~~~~~~~~~~~~~~~~~~~~
  Users may call a DBus interface to create a batch server which
  provides a lower-latency interface with a single permissions
  validation when server is created


|:electric_plug:| Extensibility
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Supports extensibility for heterogeneous environments through C++
  plugin infrastructure (IOGroups).


Overview
--------

The GEOPM Service provides a user-level interface to read telemetry
and configure settings of heterogeneous hardware platforms. Linux
system administrators may manage permissions for user access to
telemetry and configuration at a fine granularity.

Clients use the GEOPM Service DBus interface to interact with the
service.  The GEOPM Service package provides access to the DBus
interfaces from the command line, or programmatically with library
interfaces for C, C++ and Python.

The service supports many simultaneous client sessions that make
measurements, but only clients from within one Linux process session
are granted write permission to configure hardware control values at
any time.  When a client process session is granted write access it
will retain that permission until the Linux session process leader of
that client process terminates.  When the process session leader
terminates, all hardware settings that are managed by the GEOPM Service
are restored to the values they had prior to the first client write
request.  See `setsid(2) <https://man7.org/linux/man-pages/man2/setsid.2.html>`_
manual for more information about the Linux session leader process.


*
  `Overview slides <https://geopm.github.io/pdf/geopm-service.pdf>`_

Architecture
------------

.. image:: https://geopm.github.io/images/geopm-service-diagram.svg
   :target: https://geopm.github.io/pdf/geopm-service-diagram.pdf
   :alt:

The architecture diagram shows the relationship between the IOGroups
and the GEOPM Service.  IOGroups are the C++ classes that abstract
hardware interfaces like the LevelZeroIOGroup for interfacing with
Intel hardware through the LevelZero library API, or the MSRIOGroup
for interacting with the Model Specific Register device driver.  These
IOGroups provide a plugin mechanism for extending GEOPM.

The PlatformIO interface is a container for all IOGroups, and is the
primary interface for users interacting with hardware through GEOPM.
The PlatformIO interface may be accessed through language bindings
with Python, C, and C++ as well as command line tools like
``geopmread`` and ``geopmwrite``.  The GEOPM DBus interface,
``io.github.geopm``, provides the secure gateway to privileged
PlatformIO features.  The administrator uses the ``geopmaccess``
command line tool to configure the DBus interface to enable user level
access to any subset of the privileged PlatformIO features.


Status
------

The GEOPM systemd service is a new feature for version 2.0.  The
feature is fully tested and production ready.


Signals and Controls
--------------------

Each GEOPM signal and control has an associated name and a hardware
domain.  The name is a unique string identifier defined by the IOGroup
that provides the signal or control.  The hardware domain that
provides the interface is represented by one of the enumerations as
defined by PlatformTopo.  A detailed description of the signals and
controls available on a system can be discovered with the
``geopmaccess`` command line tool.  The description includes information
useful for end-users, and it also provides information for system
administrators that will help them understand what is enabled for an
end-user when access is granted to a signal or control.


Access Management
-----------------

Access to signals and controls through the GEOPM Service is configured
by the system administrator.  The administrator maintains a default
access list that applies to all users of the system.  This list
may be augmented so that users who are members of particular Unix groups may
have enhanced access.  The default lists are stored in:

.. code-block::

   /etc/geopm/0.DEFAULT_ACCESS/allowed_signals
   /etc/geopm/0.DEFAULT_ACCESS/allowed_controls


Each Unix group name ``<GROUP>`` that has extended permissions can
maintain one or both of the files

.. code-block::

   /etc/geopm/<GROUP>/allowed_signals
   /etc/geopm/<GROUP>/allowed_controls


.. note::

   Before GEOPM 3.0, service configuration files were stored in
   ``/etc/geopm-service``. Since version 3.0, they are stored in
   ``/etc/geopm``. Version 3.0 ignores the old file location if the new
   location exists. If the service uses a configuration from the old location,
   then a deprecation warning is emitted.

Any missing files are inferred to be empty lists, including the
default access files.  A signal or control will not be available to
non-root users through the GEOPM Service until a system administrator
enables access through these allow lists.  It is recommended that all
manipulation of these files should be done through the GEOPM Service
with the ``geopmaccess`` command line tool.

By convention, all control settings can be read by requesting the
signal that shares the same name as the control.  Note that when
adding a control name to the access list for writing, the
administrator is implicitly providing read access to the control
setting as well.


Opening a Session
-----------------

A client process opens a session with the GEOPM Service each time a PlatformIO
object is created with libgeopm while the GEOPM systemd service is active.
This session is initially opened in read-only mode.  Calls into the D-Bus APIs
that modify control values:

.. code-block::

   io.github.geopm.PlatformWriteControl
   io.github.geopm.PlatformPushControl


convert the session into write mode.  Only one write mode session is
allowed at any time.  The request will fail if a client attempts to
begin a write session while another client has one open.

When a session is converted to write mode, all controls that the
service is configured to support are recorded to a save directory in:

.. code-block::

   /run/geopm/SAVE_FILES


When a write mode session ends, all of these saved controls are
restored to the value they had when the session was converted,
regardless of whether or not they were adjusted during the session
through the service.

In addition to saving the state of controls, the GEOPM Service will
also lock access to controls for any other client until the
controlling session ends.  When the controlling session ends the saved
state is used to restore the values for all controls supported by the
GEOPM Service to the values they had prior to enabling the client to
modify a control.  The controlling session may end by an explicit
D-Bus call by the client, or when the process that initiated the
client session ends.  The GEOPM Service will poll procfs for the
process ID.


Batch Server
------------

The GEOPM Service provides the implementation for the ServiceIOGroup
which accesses this implementation through the DBus interface.  When a
user program calls ``read_signal()`` or ``write_control()`` on a
PlatformIO object provided by libgeopm and the only
IOGroup that provides the signal or control requested is the
ServiceIOGroup, then each request goes through the slow D-Bus
interface.  When a client process uses the ServiceIOGroup for batch
operations a separate batch server process is created through the D-Bus
interface.  The implementations for ``push_signal()`` and
``push_control()`` are used to configure the stack of signals and
controls that will be enabled by the batch server.  This batch server
interacts more directly with the client process to provide low latency
support for the ``read_batch()`` and ``write_batch()`` interfaces of the
ServiceIOGroup.

The batch server is configured to allow access to exactly the signals
and controls that were pushed onto the stack for the ServiceIOGroup
prior to the first ``read_batch()`` or ``write_batch()`` call.
Through the D-Bus implementation, the GEOPM Service verifies that the
client user has appropriate permissions for the requested signals and
controls.  When the first call to ``read_batch()`` or
``write_batch()`` is made to user's PlatformIO object, the geopmd
process forks the batch server process and no more updates can be made
to the configured requests.  The batch server uses inter-process
shared memory and FIFO special files to enable fast access to the
configured stack of GEOPM signals and controls.

To implement the ``read_batch()`` method, the ServiceIOGroup writes a
character to a FIFO to notify the batch server that it would like the
configured GEOPM signals to be updated in shared memory.  The client
process then waits on a FIFO for a message from the server that the
request is ready.  The batch server proceeds to read all GEOPM signals
that are supported by the client's ServiceIOGroup using the batch
server's instance of the PlatformIO object.  GEOPM signals are copied
into the shared memory buffer and when the buffer is ready, a
character is written into the FIFO that the client process is waiting
on.

To implement the ``write_batch()`` method, the client process's
ServiceIOGroup prepares the shared memory buffer with all control
settings that the batch server is supporting.  The client then writes
a character into a FIFO to notify the batch server that it would like
the configured GEOPM controls to be written.  The client process then
waits on a FIFO for a message from the server that the controls have
been written.  The batch server proceeds to read the clients settings
from the shared memory buffer and writes the values through the server
process's PlatformIO instance.  When the write has completed, a
character is written into the FIFO that the client process is waiting
on.
