---
#### Software Name
Global Extensible Open Power Manager (GEOPM)

---
#### Public URL
<http://geopm.github.io/geopm/>

---
#### Technical Overview

Global Extensible Open Power Manager (GEOPM) is an extensible power
management framework targeting high performance computing. The library
can be extended to support new control algorithms and new hardware
power management features. The GEOPM package provides built in
features ranging from static management of power policy for each
individual compute node, to dynamic coordination of power policy and
performance across all of the compute nodes hosting one MPI job on a
portion of a distributed computing system. The dynamic coordination is
implemented as a hierarchical control system for scalable
communication and decentralized control. The hierarchical control
system can optimize for various objective functions including
maximizing global application performance within a power bound. The
root of the control hierarchy tree can communicate through shared
memory with the system resource management daemon to extend the
hierarchy above the individual MPI job level and enable management of
system power resources for multiple MPI jobs and multiple users by the
system resource manager. The geopm package provides the libgeopm
library, the libgeopmpolicy library, the geopmctl application and the
geopmpolicy application. The libgeopm library can be called within MPI
applications to enable application feedback for informing the control
decisions. If modification of the target application is not desired
then the geopmctl application can be run concurrently with the target
application. In this case, target application feedback is inferred by
querying the hardware through Model Specific Registers (MSRs) or other
hardware interfaces. With either method (libgeopm or geopmctl), the
control hierarchy tree writes processor power policy through hardware
interfaces to enact policy decisions. The libgeopmpolicy library is
used by a resource manager to set energy policy control parameters for
MPI jobs. Some features of libgeopmpolicy are available through the
geopmpolicy application including support for static control.

The GEOPM package provides new functionality to the OpenHPC software
stack through its runtime job power management.  In order to enable
optimal power management, the GEOPM runtime also provides a
significant performance profiling framework which complements some of
the existing performance profiling tools that are provided by OpenHPC.
A significant difference between GEOPM and the other profiling tools
is that GEOPM runs on an isolated CPU, and other than removing a CPU
from the pool of available to the application, it is has a very low
impact on the performance of the application due to its remote
sampling design.

The GEOPM development is a partnership between developers and
researchers around the HPC community.  The GEOPM project is in large
part funded by an ongoing NRE grant from Argonne National Lab and we
collaborate with power researchers there to provide a path to an
exascale system with advanced power management.  We partner with the
Sandia PowerAPI group to ensure that the GEOPM interfaces are
compatible with the interfaces defined by the PowerAPI for a power
aware runtime.  We also partner with the Lawrence Livermore National
Lab power researchers to develop new power management algorithms and
extend research ideas developed there into production software.
Additionally we have a partnership with the Leibniz Supercomputing
Centre to extend the research they have done into providing an at
scale scale supercomputer while meeting strict energy efficiency
goals.  We have partnered with researchers at STFC and the Hartree
Centre to port GEOPM to the IBM Power architecture.

---
#### Latest stable version number
v0.3.0

---
#### Open-source license type
BSD 3-clause

---
#### Relationship to component?
- [x] contributing developer
- [ ] user
- [ ] other

If other, please describe:


---
#### Build system
- [x] autotools-based
- [ ] CMake
- [ ] other

If other, please describe:

Does the current build system support staged path installations?
For example: ```make install DESTIR=/tmp/foo``` (or equivalent)

- [x] yes
- [ ] no


---
#### Does component run in user space or are administrative credentials required?
- [x] user space
- [ ] admin


---
#### Does component require post-installation configuration.

- [ ] yes
- [x] no

If yes, please describe briefly:

---
#### If component is selected, are you willing and able to collaborate with OpenHPC maintainers during the integration process?
- [x] yes
- [ ] no

A fork of the OpenHPC repository that includes the GEOPM component can
be found here:

<https://github.com/cmcantalupo/ohpc/tree/cmcantalupo-geopm>

and the builds in the OpenHPC build server can be found here:

<https://build.openhpc.community/project/show/home:cmcantalupo>

---
#### Does the component include test collateral (e.g. regression/verification tests) in the publicly shipped source?
- [x] yes
- [ ] no

If yes, please briefly describe the intent and location of the tests.

The shipped source code includes unit tests based on Google Test
framework as well as integration tests build on the python py-unit
test framework.  Additionally GEOPM provides a tutorial suite which
can provide a basic smoke test.

The unit tests are here:

<https://github.com/geopm/geopm/tree/v0.3.0/test>

The intent of the unit tests is to test each class independently and
quickly with minimal system software and hardware requirements.
These tests can be executed in a simple virtual machine as is done by
our Continuous Integration process in Travis and the central OpenSUSE
Build Server for every commit integrated into our repository.

The integration tests are here:

<https://github.com/geopm/geopm/tree/v0.3.0/test_integration>

The intent of the integration tests is to prove that the features that
GEOPM provides can be shown to be valid.  These tests bring up the
full GEOPM runtime and execute a configurable synthetic application
under the control of the runtime.  The tests show that our report and
trace capabilities give consistent data, and that the goals of our
decision algorithms are met.  These tests are run nightly on our test
cluster.

And the tutorials are here:

<https://github.com/geopm/geopm/tree/v0.3.0/tutorial>

The tutorials are intended to give a new user of the GEOPM software an
introduction to the features it provides in a step by step process.
The build of the tutorials is based on a very simple Makefile system
and is independent of the rest of the GEOPM build infrastructure.  We
have integrated the build and execution of one of the tutorials as our
OpenHPC install smoke test.

---
#### Does the component have additional software dependencies (beyond compilers/MPI) that are not part of standard Linux distributions?
- [x] yes
- [ ] no

If yes, please list the dependencies and associated licenses.

There are several dependencies that can be fulfilled by packages
maintained in standard Linux distributions, however one significant
dependency is not part of the standard Linux distribution is the
msr-safe Linux kernel driver developed by Lawrence Livermore National
Lab:

<https://github.com/LLNL/msr-safe>

The msr-safe kernel driver is commonly used at a significant portion
of the major HPC facilities.  These include the Theta super-computer
at Argonne, the DOE systems running TOSS including facilities at LLNL,
LANL and Sandia, at the LRZ SuperMUC-2 system and experimentally on the
NERSC Cori system.

The msr-safe driver is targeted to HPC systems as it makes two
assumptions that are not compatible with the most general use cases of
the Linux operating system.  The first assumption is compute node
single occupancy: at any single point in time, only one user has
access to a compute node.  The second assumption is that a script can
be executed at the time the compute resources are provided to a new
user, and after the compute resources are reclaimed from that user.
Given these assumptions which are commonly met in the HPC environment,
the msr-safe driver safely enables the types of hardware measurement
and control required by GEOPM with an overhead low enough to meet the
performance requirements of the GEOPM runtime.  The msrsave utility
provided with the msr-safe driver enables save/restore functionality
to isolate user access between allocations.

---
#### Does the component include online or installable documentation?
- [x] yes
- [ ] no

If available online, please provide URL.

The GEOPM man pages are published here and are also installed as part
of the GEOPM build (see SEE ALSO section for links to the other GEOPM
man pages):

<http://geopm.github.io/geopm/man/geopm.7.html>

The GEOPM developer doxygen pages are here:

<http://geopm.github.io/geopm/dox/index.html>

The GEOPM plugin developer guide is here:

<http://geopm.github.io/geopm/pdf/geopm-plugin-developer-guide.pdf>


---
#### [Optional]: Would you like to receive additional review feedback by email?

- [x] yes
- [ ] no
