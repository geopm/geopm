GEOPM Service
=============

Features
--------

|:penguin:| Linux Integration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  The Linux Systemd Service is integrated with a DBus interface, enabling
  user-level access to hardware features on heterogeneous systems.


|:microscope:| Hardware Telemetry
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  Use a vendor-agnostic interface to read telemetry from hardware components on
  heterogeneous systems.


|:gear:| Hardware Configuration
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  User-friendly vendor-agnostic interface for configuring hardware component
  device settings on heterogeneous systems.


|:medal_sports:| Quality of Service
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  The GEOPM Service reverts any changes made to hardware configurations once
  the client's Linux process session concludes.


|:lock:| Security
~~~~~~~~~~~~~~~~~
  Linux system administrators have complete control over managing fine-grained
  access permissions for capabilities exposed by the GEOPM Service.


|:rocket:| Performance
~~~~~~~~~~~~~~~~~~~~~~
  A DBus interface can be utilized for the creation of a batch server. The
  server provides a low-latency interface with a single permissions validation
  when the server is created.


|:electric_plug:| Extensibility
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  GEOPM Service supports extensibility for heterogeneous environments through
  the C++ plugin infrastructure called IOGroups.


Overview
--------

The GEOPM Service provides an interface at the user level, allowing users to
read telemetry and configure heterogeneous hardware platforms. Linux system
administrators can manage the permissions for user access to telemetry and
configuration at very detailed level.

The GEOPM Service DBus interface is how clients interact with the service.
Access to the DBus interfaces can be gained from the command line or
programmatically with library interfaces for C, C++, and Python.

The service supports multiple simultaneous client sessions for making
measurements, however only clients from within a single Linux process session
are given write permission to configure hardware control values at any one
time. Upon termination of a client's process session leader, the GEOPM Service
restores all hardware settings to their original state before the first
client's write request.

For more information about the Linux session leader process, please consult the
`setsid(2) <https://man7.org/linux/man-pages/man2/setsid.2.html>`_ manual.

*
  `Overview slides <https://geopm.github.io/pdf/geopm-service.pdf>`_

Architecture
------------

.. image:: https://geopm.github.io/images/geopm-service-diagram.svg
   :target: https://geopm.github.io/pdf/geopm-service-diagram.pdf
   :alt:

The architectural diagram shows the relationship between IOGroups and the GEOPM
Service. IOGroups are the C++ classes that abstract hardware interfaces.
IOGroups provide a plugin mechanism to extend GEOPM's functionality.

The PlatformIO interface is a container for all IOGroups and is the main
interface for users who interact with the hardware through the GEOPM Service.
The PlatformIO interface can be accessed through language bindings with Python,
C, and C++ as well as command line tools such as ``geopmread`` and
``geopmwrite``. The GEOPM DBus interface, ``io.github.geopm``, provides a
secure gateway to privileged PlatformIO features. The ``geopmaccess`` command line
tool is used by the administrator to enable user level access to any subset of
the privileged PlatformIO features.


Status
------

The GEOPM systemd service introduced in version 2.0 is fully tested and is now
ready for production.


Signals and Controls
--------------------

Each signal and control in GEOPM has a unique name and a hardware domain. The
signals and controls available on your system can be discovered with the
``geopmaccess`` command line tool. The description includes all the necessary
information for end users and system administrators to understand what is
enabled when granting access to a signal or control.


Access Management
-----------------

System administrators configure the access to signals and controls through the
GEOPM Service. The administrator maintains an access list that applies to all
users of the system. Special Unix groups can have enhanced access.  The default
lists are stored in:

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

Any missing files are inferred to be empty lists, including the default access
files.  A signal or control will not be available to non-root users through the
GEOPM Service until a system administrator enables access through these allow
lists.  It is recommended that all manipulation of these files should be done
through the GEOPM Service with the ``geopmaccess`` command line tool.

All control settings can be read by requesting the signal with the same name.
Whenever a control name is added to the access list for writing, the
administrator implicitly grants read access to the control setting as well.


Opening a Session
-----------------

Each time a client process opens a session with the GEOPM Service, a PlatformIO
object is created with libgeopmd. This session starts in read-only mode. Calls
to the DBus APIs that modify control values convert the session into write mode.
The session retains write access until it ends. Calls into the DBus APIs that
modify control values:

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
PlatformIO object provided by libgeopmd and the only
IOGroup that provides the signal or control requested is the
ServiceIOGroup, then each request goes through the slow DBus
interface.  When a client process uses the ServiceIOGroup for batch
operations a separate batch server process is created through the DBus
interface.  The implementations for ``push_signal()`` and
``push_control()`` are used to configure the stack of signals and
controls that will be enabled by the batch server.  This batch server
interacts more directly with the client process to provide low latency
support for the ``read_batch()`` and ``write_batch()`` interfaces of the
ServiceIOGroup.

The batch server is configured to allow access to exactly the signals
and controls that were pushed onto the stack for the ServiceIOGroup
prior to the first ``read_batch()`` or ``write_batch()`` call.
Through the DBus implementation, the GEOPM Service verifies that the
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

Kubernetes Support
------------------

Experimental support for Kubernetes is provided.  The current status
of this work is a proof-of-concept, and should in no way be considered
production ready.  The ``k8-manifest.yaml`` file is a simple
demonstration that uses GEOPM in a Kubernetes environment.  This
manifest references a container that is built using a ``Dockerfile``
in this directory.  The simple ``Dockerfile`` installs the GEOPM
Service published to OpenSUSE OBS built from this branch.


Kubernetes Demo
~~~~~~~~~~~~~~~

The Kubernetes manifest creates a pod with two containers.  The first
container is privileged and is running the GEOPM service with
communication over gRPC.  The server container host mounts
``/dev/cpu``, which provides access to the ``/dev/cpu/*/msr`` device
drivers.  Also note that both containers share their PID namespace.
This is required to enable the GEOPM Service to track client process
lifetime and Unix group membership.  The client container is started
without privilege, and mounts the ``/run/geopm`` directory
which is shared with the server container.  The interprocess
communication files used by the service are located in this directory.
These files include the gRPC Unix Domain Socket (UDS) files, and also
the files for the GEOPM Service batch interfaces: the FIFO command
files, and the interprocess shared memory files.  The batch interface
enables faster communication than is available over the gRPC UDS
interface, and the UDS interfaces provide credentials for slower
interfaces.  The client communicates over gRPC to configure a batch
server.  In this way, the batch server capabilities are restricted
based on credentials verified at creation time.


Server Script
~~~~~~~~~~~~~

A human readable version of the server "command" is below:

.. code-block:: bash

   # Fix permissions on shared emptyDir{}
   chmod 711 /run/geopm
   # Update client default read access list to permit all signals
   geopmread >> /etc/geopm/0.DEFAULT_ACCESS/allowed_signals
   # Create a "client" group corresponding to the runAsGroup option
   groupadd -g 10001 client
   # Create a "client" user corresponding to the runAsUser option
   useradd -g 10001 -u 10001 client
   # Start the GEOPM daemon with gRPC communication
   geopmd --grpc


Before starting the service, the default signal allow list is
populated with *all* available signals.  There are no controls enabled
by the service.  A user and group name are created to support the UDS
credentials of the client container.  Finally the ``geopmd`` command
is run with the ``--grpc`` option.


Client Script
~~~~~~~~~~~~~

A human readable version of the client "command" is below:

.. code-block:: bash

   # Wait for server to come up (should use initContainer in k8)
   sleep 5
   # Iterate through all available signals and remove duplicates in
   # SERVICE:: namespace
   for signal in $(geopmread | grep -v SERVICE::); do
       # Print the signal name
       printf %s= ${signal}
       # Try to read and print the signal aggregated over all CPUs
       # ("board" denotes the mother-board domain)
       if geopmread ${signal} board 0 2>/dev/null; then
           # If the read was successful, add request to list
           echo ${signal} board 0 >> /tmp/geopmsession-requests.txt
       else
           # If signal cannot be read
           echo "UNAVAILABLE"
       fi
   done
   # Create a header for the CSV output (ignore dangling ',' when parsing)
   for signal in $(cat /tmp/geopmsession-requests.txt | cut -d" " -f 1); do
       printf %s, ${signal}
   done
   echo
   # Start a batch server requesting all available signals be read
   # once per second for 100 seconds. This will create a CSV output
   # with all signals read 100 times
   geopmsession -t 100 -p 1 < /tmp/geopmsession-requests.txt
   # Clean up temporary file
   rm /tmp/geopmsession-requests.txt
   # Enable users to launch a shell on the container for 1 hour
   sleep 3600



The second container uses the GEOPM Service to read all available
signals with the ``geopmread`` command line utility.  These signals
are aggregated across all CPUs on the system and the result is printed
to the client log.  Some of these read attempts may fail when a signal
is not supported by the architecture, UNAVAILABLE is printed instead
of the value.  Any read requests that succeed are added to a batch
request queue.  This queue of requests is then read once per second
and printed to the log 100 times by the ``geopmsession`` command line
tool. The ``geopmsession`` tool uses the batch interface of the GEOPM
Service to enable high speed / low overhead access to these signals.


Running the Demo
~~~~~~~~~~~~~~~~

.. code-block:: bash

   kubectl create -f k8-manifest.yaml
   kubectl logs pods/geopm-service-pod  -c geopm-client -f
