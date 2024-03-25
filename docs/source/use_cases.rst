Primary Use Cases
=================

GEOPM comes bundles various tools and software interfaces designed for broad
usability. This page outlines the multiple use cases catered by these
interfaces.


|:microscope:| Hardware Telemetry
---------------------------------

This feature enables user-level access to hardware telemetry from a wide range
of components, bridging gaps between data bandwidth and latency. Accessible
telemetry on several systems includes hardware performance counters, thermal
measurement data, and energy meters.


|:gear:| Hardware Configuration
-------------------------------

It allows user-level hardware settings adjustment without affecting other
users' quality of service of a shared system. Some potential system
configurations include setting CPU or GPU frequency limits, stipulating
processor power usage, and setting up priority cores for turbo frequency
enhancement.


|:compass:| Software Telemetry
------------------------------

This feature collects software telemetry from highly threaded distributed
applications, thereby ensuring low overhead. It provides access to real-time
telemetry for enabling online optimizations. Interfaces for reading telemetry
data come in an event log format with data burst compression and a
low-contention, low-latency sampling interface.


|:checkered_flag:| Runtime Tools
--------------------------------

A process can run on every compute node used by a High Performance Computing
(HPC) workload. Leveraging hardware and software telemetry, this process
optimally configures hardware settings. The optimization algorithm runs on a
plugin interface, easily extensible to cater to user- or site-specific needs.


|:closed_lock_with_key:| Access Management
------------------------------------------

This feature equips system administrators with a granular access management
interface. It allows them to allocate permissions for individual telemetry
readings and control settings based on Linux user group membership.
