
Primary Use Cases
=================

GEOPM comes with tools and software interfaces to access its
features. This page outlines some of the use cases that are available
across those interfaces.


|:microscope:| Hardware Telemetry
---------------------------------

Provide user-level access to hardware telemetry from heterogeneous components
with high bandwidth and low latency.  Examples of the telemetry accessible on
some systems include hardware performance counters, thermal measurements, and
energy meters.


|:gear:| Hardware Configuration
-------------------------------

Provide user-level configuration of hardware settings without impacting
quality of service for other users of a shared system.  Examples of some
system configurations that are possible include setting limits for CPU or GPU
frequency, limiting power used by processors, or configuring priority cores
for turbo frequency enhancement.


|:compass:| Software Telemetry
------------------------------

Collect software telemetry from distributed highly threaded applications while
incurring a low-overhead.  Provide access to this telemetry while the
application is active to enable online optimizations.  Interfaces to read the
telemetry are provided as an event log with compression of data bursts and
also a low-latency low-contention sampling interface.


|:checkered_flag:| Runtime Tools
--------------------------------

Run a process on each compute node used by a distributed High Performance
Computing (HPC) workload.  This process uses hardware and software telemetry
to choose optimal settings for hardware.  The algorithm used for optimization
is a plugin interface that can be extended to reflect site or user
requirements.


|:closed_lock_with_key:| Access Management
------------------------------------------

Provide system administrators with a fine-grained access management interface
that grants permissions to individual telemetry readings and control
settings based on Linux user group membership.
