
Use Cases
---------

Hardware Telemetry
~~~~~~~~~~~~~~~~~~

Provide user-level access to hardware telemetry with high bandwidth and low
latency.


Hardware Configuration
~~~~~~~~~~~~~~~~~~~~~~

Provide user-level configuration of hardware settings without impacting
quality of service for other users of a shared system.


Software Telemetry
~~~~~~~~~~~~~~~~~~

Collect software telemetry from distributed highly threaded applications while
incurring a low-overhead.  Provide access to this telemetry while the
application is active to enable online optimizations.  Interfaces to read the
telemetry are provided as an event log with compression of data bursts and
also a low-latency low-contention sampling interface.


Runtime Tools
~~~~~~~~~~~~~

Run a process on each compute node used by a distributed High Performance
Computing (HPC) workload.  This process uses hardware and software telemetry
to choose optimal settings for hardware.


Access Management
~~~~~~~~~~~~~~~~~

Provide system administrators with fine-grained access management interface
that will enable permissions to individual telemetry readings and control
settings to be granted based on Linux membership in user group, cgroup or
capability.
