Service Security
================

This document explains the objectives, mechanisms, assets and threats
associated with the design of the GEOPM Service security architecture. It
can be beneficial to system administrators, developers, and users who wish
to understand how the service provides security.


High-Level Security Objectives
------------------------------

The GEOPM Service functions as a secure gateway to privileged hardware
features. The security expectations of system administrators deploying
and configuring the service, as outlined in the GEOPM documentation, are
detailed in this section.


Fine-Grained Access Management for Hardware Features
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service provides a user gateway to privileged system software
interfaces. Typically, these interfaces have Linux mechanisms that allow
administrators to grant or deny user access. The service enhances other
Linux security mechanisms by integrating fine-grained access management,
easy extension, and an optional high-performance interface—improving upon
what typically are coarse-grained, less performant, or harder-to-extend
alternatives.

It's possible to enable certain features of existing device
drivers for end users by altering file permissions, but
this doesn't allow for the filtering of driver features. This
solution may often be insufficient due to Linux `capabilities(7)
<https://man7.org/linux/man-pages/man7/capabilities.7.html>`__ requirements.

Granting Linux capabilities to a helper command-line
tool or adding a command-line tool to the `sudoers(5)
<https://man7.org/linux/man-pages/man5/sudoers.5.html>`__ configuration is
a common method of enabling specific secure features. These command-line
tools may require independent security review and usually don't deliver
high-performance programmatic solutions.

High-performance security solutions often necessitate alterations to
Linux kernel code either through a specialized driver or via `eBPF
<https://ebpf.io>`__ program insertion. They require custom tools
integrated into the Linux kernel, requiring independent security reviews
by administrators and specialized kernel or eBPF development knowledge
from developers and security experts. Identifying errors and security
vulnerabilities in these filtering and device driver implementations can
be particularly challenging.

The GEOPM Service offers fine-grained access management to individual
hardware features for various hardware interfaces. System administrators can
grant access to each unique feature provided by a system software interface
to individual users, Unix user groups, or all system users. Expansion of
hardware interfaces is possible via the ``IOGroup`` C++ plugin interface.

"Signals" refer to hardware features that can be read and are mapped to
a particular string name. Writable hardware features, called "controls,"
are also mapped to a specific string name. These signals and controls
are represented as double-precision floating-point numbers in SI units
and are combined with a hardware domain and domain index for requests to
read signals or write controls. The hardware domain specifies the hardware
component type, while the domain index identifies the exact component where
the request applies.

The GEOPM Service adheres to a "Secure by Default" design, which means no user
access to signals or controls is provided until a system administrator grants
it. All signal and control options are documented, and their descriptions
include their security implications. The ``geopmaccess`` command-line tool
enables system administrators to query signal and control descriptions and
manage user access to each feature based on their name.


Restoring Control Configuration After a Writing Session Ends
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Users cannot make persistent changes to hardware settings through
the GEOPM Service; any configuration alterations last only for
the duration of the writing PID's process session. To understand
the Linux process session interface more, refer to the `setsid(2)
<https://man7.org/linux/man-pages/man2/setsid.2.html>`__ man page, which
explains that a process session is usually linked to a controlling terminal.

The GEOPM Service supports this feature by saving all control configuration
settings to disk before enabling a user session to write. After the user's
write session terminates, all settings are restored to their original
state. The saved state is stored in the directory ``/run/geopm/SAVE_FILES``.

Note that all configuration settings are restored, regardless of whether
they were modified during the session through the service.


One GEOPM Writing Session Allowed at Any Time
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service only supports one active writing user session at a time.
This restriction supports the save/restore methodology explained in the
previous section. The write lock enabling this mutual exclusion is tied
to the Unix "process session leader" PID of the requesting process. If
this session leader PID doesn't refer to an active process, the lock
is associated with the requesting process PID. Otherwise, any PID within
the requesting Unix process session can write via the GEOPM Service for
the entire process session leader PID lifespan.


Providing a Low-Latency Interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service offers a batch interface letting users configure
a server process. The batch server process is forked by ``geopmd``,
the primary GEOPM Service daemon process started by `systemd(1)
<https://man7.org/linux/man-pages/man1/systemd.1.html>`__.  The user can
configure the batch server with an array of requests via DBus. Access
permissions are checked when initiating the server, which will only be
forked if the client has authorization for all array requests.  The batch
server establishes one or two inter-process shared memory regions to
communicate with the client — one for user interface signal readings,
and another (if requested) to provide the user interface control for
writing.  The client makes requests to read, write, or quit through
a FIFO channel, connecting them to the batch server. A second FIFO
communicates to the client when requests are complete. These FIFOs are
opened in ``/tmp``, which systemd sets up by default as a `tmpfs(5)
<https://man7.org/linux/man-pages/man5/tmpfs.5.html>`__.


Securing Restart of the ``geopmd`` Daemon Process
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All states required to uphold the GEOPM Service's security guarantees
are stored securely on disk (for more details, see :ref:`security:File
Usage/Configuration`). The ``geopmd`` daemon process reads this information
from the disk on startup, allowing the service to recover cleanly in the
event the daemon process ends prematurely. The systemd Linux service offers
a restart of the daemon process through a configuration option provided
by the GEOPM Service. The service may be restarted immediately, with the
option to retry twice in quick succession if required. The first attempt
resets the environment for the second restart.


Secure Interfaces
-----------------

Users can access privileged system resources through the GEOPM Service via the
GEOPM DBus interface published on the system bus at ``io.github.geopm``. This
interface can facilitate the creation of a batch server. The batch server's
access rights are verified before it's created, and the user can then interact
with it via faster mechanisms than those provided by DBus. In particular,
the user interacts with the batch server over inter-process shared memory
and FIFO special files, created in ``/run/geopm``.

The DBus interface offers a security layer used throughout Linux services
to confirm the user identity of daemon process requests. The GEOPM Service
relies on the systemd DBus interface to provide PID, UID, and GID details
of the requesting client. These identifiers are then used with standard
system calls to enforce access permissions defined by a system administrator.


Secure Software Dependencies
----------------------------

The GEOPM Service relies on external software packages to support its security
objectives. These packages allow secure use of the DBus interface to systemd,
providing standard methods for validating JSON data.

1. GEOPM Service DBus Interface

   a. dasbus >= 1.6

   b. libsystemd.so / systemd service > 234

   c. PyGObject >= 3.34.0

2. GEOPM Service Input/Output Validation

   a. jsonschema >= 2.6.0

   b. json11 >= 1.0.0


Protected Assets
----------------

The GEOPM Service operates as a secure passageway to privileged hardware
interfaces, including power and energy management features and performance
counter readings. These secure system software interfaces, accessible
through the GEOPM Service, are explained in this section. The interfaces
can also be expanded using the GEOPM IOGroup plugin interface.

For system administrators to manage access to hardware features, the GEOPM
service provides the ``geopmaccess`` command line interface. The interface
is expected to be a reliable and secure means of managing users' access
rights to the assets discussed in this section. User privacy maintenance,
ensuring that the GEOPM service interactions with the client are not visible
to other users, is also a security priority.


Model-Specific Register Device Driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service can function as a gateway to the x86 Model-Specific Register
device driver, `msr(4) <https://man7.org/linux/man-pages/man4/msr.4.html>`__,
loaded as ``/dev/cpu/*/msr`` devices. There are various features accessible
through the MSR device driver, and the GEOPM Service allows usage of a subset
of these features focused on energy and power management and performance
monitoring. Examples include reading instruction counters or setting CPU
core operating frequency limits.

As direct access to the MSR driver may enable users to gain
unauthorized information about processes they don’t own or
influence system performance for other users, it's restricted. Using
the MSR driver requires the ``CAP_SYS_RAWIO`` Linux `capability
<https://man7.org/linux/man-pages/man7/capabilities.7.html>`__.

The GEOPM Service’s access management system enables a system
administrator to control which features can be accessed through the MSR
driver. The service also prevents permanent changes to the MSR driver. As
such, administrators may want to provide MSR access through the GEOPM
Service to processes that lack the ``CAP_SYS_RAWIO`` Linux `capability
<https://man7.org/linux/man-pages/man7/capabilities.7.html>`__.


Intel Speed Select Device Driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service can act as a gateway to the Intel Speed Select device
driver, loaded as the ``/dev/isst_interface`` device. This driver offers a
broad set of capabilities introduced with the 3rd generation Xeon Scalable
server processor.

https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html

The specific features supported through the GEOPM Service are the `SST-CP
<https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html#intel-r-speed-select-technology-core-power-intel-r-sst-cp>`__
and `SST-TF
<https://www.kernel.org/doc/html/latest/admin-guide/pm/intel-speed-select.html#intel-r-speed-select-technology-turbo-frequency-intel-r-sst-tf>`__
features. Use of the ``isst_interface`` device driver necessitates the Linux
`capability <https://man7.org/linux/man-pages/man7/capabilities.7.html>`__
of ``CAP_SYS_ADMIN`` because changes may influence system performance for
other users. The ISST interface can also alter the hardware characteristics
reported by the Linux kernel, including the number of cores, base frequency,
and achievable turbo frequencies.


LevelZero Sysman Library Interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The LevelZero sysman library interface allows users to monitor and control
Intel GPU devices. These signals and controls include setting GPU operating
frequency bounds and reading performance counters from GPU devices. Access
to the LevelZero sysman interface is restricted as it provides the ability
to alter system performance and direct access to hardware metrics that
reflect user activity.


Nvidia NVML Device Management Library Interface
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The NVML library interface allows users to monitor and control Nvidia GPU
devices. Controls and monitors for setting GPU operating frequency bounds
and reading performance counters from GPU devices are available. Access to
some NVML library interfaces is restricted, but some restrictions may be
relaxed based on settings enabled by a system administrator.


User Data
^^^^^^^^^^^^^^^^^

Any interaction between each client and the GEOPM Service is considered
private information and should be protected. Therefore, unprivileged users
should not be able to observe the calls, inputs, or outputs made to the
GEOPM Service by other users.


Attack Surface
--------------

This section outlines the interfaces that must be secure to maintain the
security requirements of the GEOPM Service.


System Files
^^^^^^^^^^^^

The state used to manage access permissions, track active sessions, and store
control settings for reset is maintained in system files. Files controlling
access permissions are in the ``/etc/geopm`` directory. Information necessary
to support active user sessions is stored in ``/run/geopm``. Protecting
these files is crucial to the GEOPM Service security model. Generally,
these files are only accessible by root and are modified by interacting with
GEOPM Service interfaces or running GEOPM Service command-line tools such as
``geopmaccess``.


Inter-Process Shared Memory
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The batch server interface of the GEOPM Service uses POSIX inter-process
shared memory to communicate with user processes. For every opened batch
session by a user process, one or two shared memory regions are created
for communication. Protecting these shared system resources is a critical
aspect of our security model. Unauthorized access to these shared memory
regions by a user other than the client may lead to privilege escalation
and disruption of the administrative access lists.


FIFO Special Files
^^^^^^^^^^^^^^^^^^

To support GEOPM Service's batch server features, FIFO special files are
created in the ``/tmp`` directory, working in tandem with inter-process
shared memory. These FIFOs act as synchronization mechanisms, facilitating
notifications between the client and server regarding shared memory data
updates. Unauthorized access to these files might result in batch server or
client process deadlocks and potential exposure of client session request
details.

Systemd DBus Interface
^^^^^^^^^^^^^^^^^^^^^^

GEOPM Service leverages the systemd DBus, a standardized
Linux interface for secure service communication. The `sd-bus(3)
<https://man7.org/linux/man-pages/man3/sd-bus.3.html>`__ interface of the
Linux systemd service enables secure request and result exchanges with the
GEOPM Service, as well as the identification of client request origins. On the
server side, the GEOPM DBus interface implementation utilizes the :doc:`dasbus
<dasbus:index>` and :doc:`PyGObject <pygobject:index>` Python modules. In
contrast, the client side employs ``libsystemd.so`` with the `sd-bus(3)
<https://man7.org/linux/man-pages/man3/sd-bus.3.html>`__ interface. The GEOPM
Service trusts these standard Linux tools for a reliable and secure interface.

Logging
^^^^^^^

GEOPM prioritizes comprehensive logging to ensure traceability for system
administrators regarding user activities. Emphasis is placed on recording
security-sensitive events, limiting excessive logging, protecting private
information, and maintaining log integrity. The GEOPM Service can write logs
via the ``dasbus`` provided service. Typically, logs are available through
the systemd-supported journalctl command (e.g., ``journalctl -u geopm``)
or by inspecting ``/var/log/messages``, though access might vary depending
on system configuration. Additionally, any errors arising from the setup or
usage of secure configuration files will be logged. More details on these
secure files can be found at :ref:`security:System files`.

Security Threats
----------------

This section outlines potential threats to the GEOPM Service's security,
detailing how each threat might exploit vulnerabilities and the measures
taken to fortify against them.

Malicious Input or Private Output
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Unprivileged user-accessible GEOPM Service interfaces represent potential
threat vectors. All user inputs are scrutinized to prevent the use of harmful
or incorrect data that might compromise or misconfigure the system. Similarly,
all outputs are checked to prevent the disclosure of private or malicious
data. Two main interfaces are available to end users: the ``io.github.geopm``
DBus interface via systemd and the batch server interface accessible through
inter-process shared memory and FIFO special files in ``/tmp``.

File Usage/Configuration
^^^^^^^^^^^^^^^^^^^^^^^^

To ensure data security as GEOPM reads/writes configuration files, several
precautions are taken. These include confirming that input files aren't
misleading symbolic links, ensuring all temporary files are appropriately
managed, and verifying the security settings of used temporary files
and directories. GEOPM uses disk files for various purposes, such as
facilitating user/group access to privileged signals/controls, storing
active client session data, and saving initial hardware control states for
potential reversion. Temporary files and move operations ensure complete and
valid data write operations to secure locations. GEOPM also takes measures
to counter threats by extensively inspecting files/directories intended
for input. GEOPM doesn't provide user-facing APIs that accept paths. All
directory paths are hard-coded within the GEOPM Service. Comprehensive
information on file and directory security can be found in `system_files.py
<https://github.com/geopm/geopm/blob/dev/service/geopmdpy/system_files.py>`__.

External Dependencies
^^^^^^^^^^^^^^^^^^^^^

The GEOPM Service relies on shared libraries for user plugins related to
IOGroups and Agents. These plugins are expected to be in a specific disk
path set by system administrators. Only validated shared objects in this
designated location are loaded during service startup and used upon user
request. By default, the ``GEOPM_PLUGIN_PATH`` environment variable isn't
exported before launching ``geopmd``, disabling this feature. GEOPM also
uses third-party JSON libraries for C/C++ runtime and multiple Python modules
for the GEOPM Service. Nightly integration tests ensure the latest versions
of these external Python modules function as expected, with any issues being
promptly reported to developers. For C/C++ JSON usage, the upstream repository
is regularly checked to confirm the GEOPM-hosted code remains current.

