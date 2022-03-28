
Guide to Service Security
=========================

This documentation describes the objectives, mechanisms, assets and
threats that were considered when designing the GEOPM Service security
architecture.  This information may be useful for system
administrators, developers, and users who would like to better
understand how the GEOPM Service provides security guarantees.


High Level Security Objectives
------------------------------

The GEOPM Service provides a secure gateway to privileged hardware
features. This section describes the security expectations of the
system administrator who deploys and configures the GEOPM Service as
described in the GEOPM documentation.


Fine-grained access management for hardware features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service provides a user gateway to privileged system
software interfaces. In general, these system software interfaces have
other Linux mechanisms for system administrators to grant or deny user
access.  The GEOPM Service improves upon other available Linux
security mechanisms by combining fine-grained access management, ease
of extension, and an optional high performance interface. Alternative
options available tend to be more course-grained, less performant, or
more difficult to extend.

Some features of existing device drivers may be enabled for
end users by changing file permissions, however this does not allow
for filtering of which driver features will be available, and is often
insufficent due to Linux
`capabilities(7) <https://man7.org/linux/man-pages/man7/capabilities.7.html>`__
requirements.

Granting Linux capabilites to a helper command line tool or adding a
command line tool to the
`sudoers(5) <https://man7.org/linux/man-pages/man5/sudoers.5.html>`__
configuration is a common way to filter which secure features are
enabled.  These command line tools may also require independent
security review and typically do not provide a high performance
programmatic solution.

High performance security solutions tend to require modifications to
Linux kernel code through either a specialized driver, or through the
insertion of `eBPF <https://ebpf.io>`__ programs.  These solutions
require installation of custom tools into the Linux kernel that may
require independent security review by administrators.  The developers
and security experts must have specialized knowledge of the kernel or
eBPF development environment.  Errors and security vulnerabilities in
these filtering and device driver implementations may be more
difficult to identify.

The GEOPM Service provides the system administrator with fine-grained
access management for individual hardware features for a variety of
hardware interfaces. Access to each individual feature provided by a
system software interface may be granted to individual users, or to
Unix user groups, or to all users of the system.  The hardware
interfaces may be expanded through the ``IOGroup`` C++ plugin
interface.

Each hardware feature that may be read is referred to as a "signal"
and mapped to a specific string name. Each hardware feature that may
be written is referred to as a "control" and is mapped to a specific
string name. These signals and controls are represented as double
precision floating point numbers in SI units. Requests to GEOPM
interfaces to read a signal or write a control are combined with a
hardware domain and a domain index.  The hardware domain specifies the
type of hardware component and the domain index identifies the
specific component where the request will be applied.

The GEOPM Service does not provide user access to any signals or
controls until a system administrator grants this access; this follows
the "Secure by Default" design. All available signals and controls are
documented, and the description includes the security implications of
granting access. The ``geopmaccess`` command line tool is used by a
system administrator to query descriptions of signals and controls
available on a system as well as managing user access to each feature
by name.


Restore control configuration after a writing session ends
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Users are not allowed to make persistent changes to hardware settings
using the GEOPM Service. The lifetime of configuration changes made
through the GEOPM Service is limited to the lifetime of the writing
PID's process session.  To learn more about the Linux process session
interface see the
`setsid(2) <https://man7.org/linux/man-pages/man2/setsid.2.html>`__
man page.  Typically the process session is coupled to a controlling
terminal.

The GEOPM Service implements this feature by saving all control
configuration settings to disk prior to enabling a user session to
write. When the user's write session terminates, all configuration
settings are restored to their previous value. The storage location
for the saved state is in the directory
``/var/run/geopm-service/SAVE_FILES``.

Note that all configuration settings are restored regardless of
whether or not they were adjusted during the session through the
service.


One writing GEOPM session allowed at any time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service enforces that at most one writing user session is
supported at any time.  This restriction enables the save/restore
methodology outlined in the previous section to be effective. The
write lock that enables this mutual exclusion is associated with the
Unix "process session leader" PID of the requesting process. If this
session leader PID does not refer to an active process, then the lock
is associated with the requesting process PID. Otherwise, any PID
within the requesting Unix process session is enabled to write through
the GEOPM Service for the duration of the process session leader PID
lifetime.


Provide a low latency interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service provides a batch interface that allows users to
configure a server process.  The batch server process is forked by
``geopmd`` which is the primary GEOPM Service daemon process started
by
`systemd(1) <https://man7.org/linux/man-pages/man1/systemd.1.html>`__.
The user configures the batch server with an array of requests through
DBus. The access permissions are checked when the call to start the
server is made, and the server is only forked if the client has
permission for all requests in the array. The batch server creates one
or two inter-process shared memory regions to interact with the
client: one region provides a user interface to read signals, and the
other if requested provides a user interface to write
controls. Requests to read, write, or quit are made through a FIFO
channel connecting the client to the batch server, and a second FIFO
is used to communicate to the client when requests have been
completed. These FIFOs are opened in ``/tmp`` which by default systemd
creates as a `tmpfs(5)
<https://man7.org/linux/man-pages/man5/tmpfs.5.html>`__.


Secure restart of the ``geopmd`` daemon process
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All state that is required to maintain the security guarantees of the
GEOPM Service are stored to disk in a secure way (see
:ref:`File Usage/Configuration`
for more details).  This information is read from disk when the
``geopmd`` daemon process begins. This enables the service to cleanly
recover from any premature ending of the daemon process. The restart
of the daemon process is enabled by the systemd Linux service though a
configuration option that the GEOPM Service provides. These options
enable the GEOPM Service to be restarted immediately, and to attempt
to restart twice in rapid succession if necessary. The first attempt
may be used to provide a clean environment for the second restart.


Secure Interfaces
-----------------

The interfaces that enable a user to access privileged system
resources through the GEOPM Service are limited to the GEOPM DBus
interface published on the system bus at ``io.github.geopm``. Using
this interface may enable the creation of a batch server. The access
rights of this batch server are verified prior to its creation, and
the user may then interact with this batch server through faster
mechanisms than DBus provides. In particular, the user interfaces with
the batch server over inter-process shared memory through the
``/dev/shm`` device, and sends commands through FIFO special files in
``/tmp``.

The DBus interface provides a layer of security that is leveraged
throughout Linux services to verify the user identity of requests made
to daemon processes. The GEOPM Service relies on the systemd DBus
interface to provide the PID, UID, and GID of the requesting client.
These identifiers are then used with standard system calls to enforce
access permissions defined by a system administrator.


Secure Software Dependencies
----------------------------

The GEOPM Service relies on external software packages to support
security objectives. These external packages enable secure use of the
DBus interface to systemd and provide standard methods for verifying
JSON data.

1. GEOPM Service DBus Interface

   a. dasbus >= 1.6

   b. libsystemd.so / systemd service > 234

   c. PyGObject >= 3.34.0

2. GEOPM Service Input/Output Validation

   a. jsonschema >= 2.6.0

   b. json11 >= 1.0.0


Protected Assets
----------------

The GEOPM Service provides a security gateway to privileged hardware
interfaces. These interfaces expose power and energy management
features as well as hardware monitoring features such as reading
performance counters. The secure system software interfaces that are
available through the GEOPM Service are described in this
section. These interfaces may also be expanded by using the GEOPM
IOGroup plugin interface.

The GEOPM Service provides the ``geopmaccess`` command line interface
for system administrators to manage access to hardware features. This
interface is expected to be a reliable and secure mechanism for
managing access rights of users to the assets described in this
section.  Maintaining user privacy such that client interactions with
the GEOPM service are not observable to other users is also a security
priority.


Model Specific Register device driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service may be used as a gateway to the x86 Model Specific
Register device driver
`msr(4) <https://man7.org/linux/man-pages/man4/msr.4.html>`__
which is loaded as the ``/dev/cpu/*/msr`` devices.  There are many
features available through the MSR device driver, and the GEOPM
Service provides access to a subset of these features. The features
supported by the GEOPM Service are focused on power and energy
management as well as performance monitoring. Some examples are
reading instruction counters or setting limits on CPU core operating
frequency.

Direct access to the MSR driver is restricted as this may enable users
to obtain information about processes they do not own or impact system
performance for other users. For these reasons using the MSR driver
requires the CAP_SYS_RAWIO Linux
`capability <https://man7.org/linux/man-pages/man7/capabilities.7.html>`__.

The GEOPM Service access management system allows a system
administrator to control precisely which subset of features available
through the MSR driver may be accessed. Additionally, the GEOPM
Service does not allow persistent changes to the MSR driver. For these
reasons a system administrator may wish to provide MSR access through
the GEOPM Service to processes that do not have the CAP_SYS_RAWIO
Linux
`capability <https://man7.org/linux/man-pages/man7/capabilities.7.html>`__.


Intel Speed Select device driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service may be used as a gateway to the Intel Speed Select
device driver which is loaded as the ``/dev/isst_interface``
device. This device driver enables a wide range of capabilities
introduced with the 3rd generation Xeon Scalable server processor.

https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html

The specific features enabled through the GEOPM Service are the
`SST-CP <https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html#intel-r-speed-select-technology-core-power-intel-r-sst-cp>`__
and
`SST-TF <https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html#intel-r-speed-select-technology-turbo-frequency-intel-r-sst-tf>`__
features. Using the ``isst_interface`` device driver requires the
Linux
`capability <https://man7.org/linux/man-pages/man7/capabilities.7.html>`__
of CAP_SYS_ADMIN because changes may impact system performance for
other users of the system. The ISST interface may also be used to
change the hardware characteristics reported by the Linux kernel, such
as number of cores, base frequency, and which turbo frequencies are
achievable.


LevelZero sysman library interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The LevelZero sysman library interface enables users to monitor and
control Intel accelerator devices. These signals and controls include
setting bounds on GPU operating frequency and reading performance
counters from GPU devices. Access to the LevelZero sysman interface is
restricted because it provides ability to modify the performance of
the system and direct access to hardware metrics that reflect user
activity.


Nvidia NVML device management library interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The NVML library interface enables users to monitor and control Nvidia
GPU devices. These controls and monitors include setting bounds on GPU
operating frequency and reading performance counters from GPU devices.
Access to some of the interfaces of the NVML library are restricted
and some of those restrictions may be relaxed based on settings
enabled by a system administrator.


Service user data
^^^^^^^^^^^^^^^^^

Interactions with the GEOPM Service by each client are private
information and an asset that the GEOPM Service is designed to
protect.  Unprivileged users of the system should not be able to
observe the calls, inputs, or outputs to the GEOPM Service by other
users of the system.


Attack Surface
--------------

This section describes the interfaces that must be protected to ensure
the security requirements of the GEOPM Service.


System files
^^^^^^^^^^^^

The state used to manage access permissions, track open sessions, and
store control settings for reset is stored in system files. The files
controlling access permissions are located in the
``/etc/geopm-service`` directory. The state required to support active
user sessions is stored in ``/var/run/geopm-service``. Protecting
these files is paramount to the integrity of the GEOPM Service
security model.  In general these files have root access permissions
only, and are modified by calling into GEOPM Service interfaces, or by
running GEOPM Service command line tools like ``geopmaccess``.


Inter-process shared memory
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The batch server interface of the GEOPM Service uses POSIX
inter-process shared memory to communicate telemetry and
configurations with a user processes. For each batch session opened by
a user process there are one or two shared memory regions opened for
communication. Protecting the integrity of these shared system
resources is a critical part of our security model. Access to these
shared memory regions by a user other than the client can result in
escalation of privileges, and subversion of the administrative access
lists.


FIFO special files
^^^^^^^^^^^^^^^^^^

In conjunction with the inter-process shared memory, there are FIFO
special files created in ``/tmp``, the temporary file system, to
support the batch server features of the GEOPM Service. Notifications
sent between the client and server about updates to data in
inter-process shared memory go across the FIFOs which serve as a
synchronization mechanism.  Adversarial access to these files could
cause deadlocks in the batch server or client process or enable
information about the client session requests to be observed.


Systemd DBus interface
^^^^^^^^^^^^^^^^^^^^^^

The systemd DBus implementation is a standard Linux interface for
secure communication with Linux system services.  The
`sd-bus(3) <https://man7.org/linux/man-pages/man3/sd-bus.3.html>`__
DBus interface of the Linux systemd service provides the mechanism for
users to securely exchange requests and results with the GEOPM
Service. The DBus interface also enables the GEOPM Service to securely
identify the source of client requests. The GEOPM implementation uses
the
`dasbus <https://dasbus.readthedocs.io>`__
and
`PyGObject <https://pygobject.readthedocs.io>`__
Python modules to implement the server side of the GEOPM DBus
interface in Python, while it uses the libsystemd.so to implement a C
interface to the client side of the GEOPM DBus interface directly with
the
`sd-bus(3) <https://man7.org/linux/man-pages/man3/sd-bus.3.html>`__
interface.  The GEOPM Service relies on these standard Linux tools to
provide a trusted interface and a secured attack surface.


Logging
^^^^^^^

GEOPM's implementation ensures that GEOPM has sufficient logging to
provide traceability to system administrators about user
interactions. This includes logging of security critical events (e.g.
user interactions that result in changing the system configuration),
avoiding excessive logging, avoiding public logging of private
information and ensuring logs are reliable.

The GEOPM Service has the capability of writing to the logging service
provided by ``dasbus``. Most commonly this is the logging capabilities
provided by systemd, and are accessible via the journalctl command
(e.g.  journalctl -u geopm) or through inspection of
``/var/log/messages`` or similar ways of viewing the ``syslog`` which
may depend on how the system is configured.

The GEOPM Service will log any error conditions that arise from
attempting to set up or use configuration files stored in a secure
location.  See
:ref:`System files`
for more information about these secure files.


Security Threats
----------------

This section enumerates the mechanisms that an adversary may use in an
attempt to breach the attack surface to subvert the security
guarantees of the GEOPM Service. Each threat is split out into its own
subsection.  The threat is described in the context of how the
mechanism might be used against the GEOPM Service attack surface, and
how the surface is secured against this threat.


Malicious input or private output
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Each GEOPM Service interface that may be accessed by an unprivileged
user is a threat vector.  All input recieved from the user is
validated to ensure no malicious or malformed data is used in any way
that may result in a compromised or misconfigured system. All output
from these interfaces is vetted to ensure that the service does not
emit private, malicious, or malformed data.

The GEOPM Service provides two interfaces for end users.  One is the
``io.github.geopm`` DBus interface published through systemd.  The
other is the batch server interface which may be created with a
request to the GEOPM DBus interface.  The batch server is accessed by
the end user through inter-process shared memory, and FIFO special
files in the temporary file system.


File usage/configuration
^^^^^^^^^^^^^^^^^^^^^^^^

As GEOPM reads and writes configuration files to disk, it is important
to validate that the file usage is done as securely as possible. This
includes verifying an input file is not a symbolic link to an
unintended resource, verifying that all temporary files are cleaned up
properly, and that the temporary files and directories that are used
do not have excessive permissions.

The GEOPM Service utilizes files on disk to support several behaviors
including facilitating user/group access to privileged
signals/controls, storing state information about in-progress client
sessions, and saving the initial state of hardware controls so that
any control changes may be reverted. GEOPM utilizes temporary files
and move semantics to ensure that files written to the secure
locations previously described are complete and have valid data.

GEOPM mitigates threats in this space by performing several checks on
any files/directories used for input. Note that there are no user
facing APIs provided by the GEOPM Service that take paths as
input. All directory paths used in the GEOPM Service are statically
defined in the source code.

A secure API for dealing with files and directories resides in
`system_files.py <https://github.com/geopm/geopm/blob/dev/service/geopmdpy/system_files.py>`__.
The functions that match the pattern system_files.secure_*() are the
only interfaces called by the GEOPM Service to access files located in
``/etc`` and ``/var/run``. These secure functions are used to make
directories and any input or output to these system files.

When making directories, if the path already exists checks are
performed to ensure: the path is a regular directory, the path is not
a link, the path is accessible by the caller, the path is owned by the
calling process UID/GID, and the permissions on the directory are set
to the right permissions (chosen to be as restrictive as possible). If
the path is determined to be insecure, the existing path is renamed to
indicate it is invalid and preserved for later auditing. In this case
a new directory will be created at the specified path. If the path did
not already exist, a new directory is created with the proper
permissions.

By default, directories are created with 0o700 permissions (i.e. rwx
only for the owner). Some directories, for example
``/var/run/geopm-service``, also require execution permissions (i.e.
0o711). For more details on how directories are created and default
permissions, please see the `system_files.py <http://geopm.github.io/geopmdpy.7.html#module-geopmdpy.system_files>`__
documentation

When making files, a temporary file is first created with 0o600 or
owner rw only permissions. The desired contents are then written to
this temporary file. Once writing is complete, the temporary file is
renamed to the desired path while preserving the 0o600
permissions. This rename is atomic, so it is not possible for files to
exist with partial/corrupt data. Any existing file at the desired
location will be overwritten.

When reading files, first the path's security is verified.  The
implementation asserts that the path describes an existing regular
file which is not a link nor a directory.  After the path is verified,
a file descriptor is opened referencing the path and this file
descriptor's security is verified.  The implementation asserts that
the descriptor refers to a regular file owned by the calling process
UID/GID and that the file descriptor has minimal permissions
(i.e. 0o600 or rw for the owner only).  After these assertions have
been made, the implementation reads the entire file contents into a
string buffer and the file descriptor is closed.


External dependencies
^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service utilizes shared libraries to facilitate user plugins
of the IOGroups and Agents. The service expects the plugins to reside
in a particular path on disk that can only be set by the system
administrator. If valid shared objects reside in the prescribed
location, they will be loaded at service startup and utilized if
requested by the user. The expectation is that a system administrator
would set the plugin path and only place vetted plugins there if
needed.  The GEOPM Service is a systemd service unit which is
configured through the
`systemd.service(5) <https://man7.org/linux/man-pages/man5/systemd.service.5.html>`__
file.  The configuration file provided with the GEOPM source code,
`geopm.service <https://github.com/geopm/geopm/blob/dev/service/geopm.service>`__,
does not export the ``GEOPM_PLUGIN_PATH`` environment variable before
launching ``geopmd``, so this feature is disabled by default.

GEOPM makes use of third-party JSON libraries in the C/C++ runtime,
and several Python modules in the case of the GEOPM Service. Through
the course of the nightly integration testing of GEOPM, the latest
released versions of all external Python modules are installed via
pip. If any issues arise, reports are automatically generated to
detail the failure and are sent to the developers. For the C/C++ JSON
usage, the upstream repository is periodically checked to ensure the
code that resides in GEOPM is up-to-date.
