#  Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
Summary: Global Extensible Open Power Manager
Name: geopm
Version: 0.3.0+dev75g722d7a3
Release: 1
License: BSD-3-Clause
Group: System Environment/Libraries
Vendor: Intel Corporation
URL: http://geopm.github.io/geopm
Source0: geopm.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: gcc-c++
BuildRequires: python
BuildRequires: openmpi-devel
BuildRequires: hwloc-devel
BuildRequires: unzip
BuildRequires: libtool

%if 0%{?suse_version} >= 1320
BuildRequires: openssh
%endif
BuildRequires: python-devel

Prefix: %{_prefix}

%if %{defined suse_version}
%define docdir %{_defaultdocdir}/geopm
%else
%define docdir %{_defaultdocdir}/geopm-%{version}
%endif

%description
Global Extensible Open Power Manager (GEOPM) is an extensible power
management framework targeting high performance computing.  The library can be
extended to support new control algorithms and new hardware power management
features.  The GEOPM package provides built in features ranging from static
management of power policy for each individual compute node, to dynamic
coordination of power policy and performance across all of the compute nodes
hosting one MPI job on a portion of a distributed computing system.  The
dynamic coordination is implemented as a hierarchical control system for
scalable communication and decentralized control.  The hierarchical control
system can optimize for various objective functions including maximizing
global application performance within a power bound.  The root of the control
hierarchy tree can communicate through shared memory with the system resource
management daemon to extend the hierarchy above the individual MPI job level
and enable management of system power resources for multiple MPI jobs and
multiple users by the system resource manager.  The geopm package provides the
libgeopm library, the libgeopmpolicy library, the geopmctl application and the
geopmpolicy application.  The libgeopm library can be called within MPI
applications to enable application feedback for informing the control
decisions.  If modification of the target application is not desired then the
geopmctl application can be run concurrently with the target application.  In
this case, target application feedback is inferred by querying the hardware
through Model Specific Registers (MSRs).  With either method (libgeopm or
geopmctl), the control hierarchy tree writes processor power policy through
MSRs to enact policy decisions.  The libgeopmpolicy library is used by a
resource manager to set energy policy control parameters for MPI jobs.  Some
features of libgeopmpolicy are available through the geopmpolicy application
including support for static control.

%prep

%setup

%package devel
Summary: Global Extensible Open Power Manager - development
Group: Development/Libraries
Requires: geopm

%description devel
Development package for GEOPM.

%package -n python-geopmpy
Summary: Global Extensible Open Power Manager - python
Group: System Environment/Libraries
Requires: geopm
%{?python_provide:%python_provide python-geopmpy}

%description -n python-geopmpy
Python package for GEOPM.

%build
test -f configure || ./autogen.sh

%if %{defined suse_version}
./configure --prefix=%{_prefix} --libdir=%{_libdir} --libexecdir=%{_libexecdir} \
            --includedir=%{_includedir} --sbindir=%{_sbindir} \
            --mandir=%{_mandir} --docdir=%{docdir} \
            --with-mpi-bin=%{_libdir}/mpi/gcc/openmpi/bin \
            --disable-fortran --disable-doc \
            || ( cat config.log && false )
%else
./configure --prefix=%{_prefix} --libdir=%{_libdir} --libexecdir=%{_libexecdir} \
            --includedir=%{_includedir} --sbindir=%{_sbindir} \
            --mandir=%{_mandir} --docdir=%{docdir} \
            --with-mpi-bin=%{_libdir}/openmpi/bin \
            --disable-fortran --disable-doc \
            || ( cat config.log && false )
%endif

%{__make}

%check

%if %{defined suse_version}
LD_LIBRARY_PATH=/usr/lib64/mpi/gcc/openmpi/lib \
MPIEXEC=/usr/lib64/mpi/gcc/openmpi/bin/mpiexec %{__make} check || \
( cat test/gtest_links/*.log && cat scripts/test/pytest_links/*.log && false )
%else
LD_LIBRARY_PATH=/usr/lib64/openmpi/lib \
MPIEXEC=/usr/lib64/openmpi/bin/mpiexec %{__make} check || \
( cat test/gtest_links/*.log && cat scripts/test/pytest_links/*.log && false )
%endif

%install
%{__make} DESTDIR=%{buildroot} install
rm -f $(find %{buildroot}/%{_libdir} -name '*.a'; \
        find %{buildroot}/%{_libdir} -name '*.la')
rm -f %{buildroot}/%{_mandir}/man1/geopmpy_launcher.1*

%clean

%post
/sbin/ldconfig

%preun

%postun
/sbin/ldconfig

%files
%defattr(-,root,root,-)
%{_libdir}/libgeopmpolicy.so.0.0.0
%{_libdir}/libgeopmpolicy.so.0
%{_libdir}/libgeopmpolicy.so
%{_libdir}/libgeopm.so.0.0.0
%{_libdir}/libgeopm.so.0
%{_libdir}/libgeopm.so
%dir %{_libdir}/geopm
%{_libdir}/geopm/libgeopmpi_governing.so.0.0.0
%{_libdir}/geopm/libgeopmpi_governing.so.0
%{_libdir}/geopm/libgeopmpi_governing.so
%{_libdir}/geopm/libgeopmpi_balancing.so.0.0.0
%{_libdir}/geopm/libgeopmpi_balancing.so.0
%{_libdir}/geopm/libgeopmpi_balancing.so
%{_libdir}/geopm/libgeopmpi_simplefreq.so.0.0.0
%{_libdir}/geopm/libgeopmpi_simplefreq.so.0
%{_libdir}/geopm/libgeopmpi_simplefreq.so
%{_libdir}/geopm/libgeopmpi_mpi.so.0.0.0
%{_libdir}/geopm/libgeopmpi_mpi.so.0
%{_libdir}/geopm/libgeopmpi_mpi.so
%{_bindir}/geopmbench
%{_bindir}/geopmctl
%{_bindir}/geopmpolicy
%dir %{docdir}
%doc %{docdir}/README
%doc %{docdir}/COPYING
%doc %{docdir}/VERSION
%doc %{_mandir}/man1/geopmbench.1.gz
%doc %{_mandir}/man1/geopmctl.1.gz
%doc %{_mandir}/man1/geopmpolicy.1.gz
%doc %{_mandir}/man3/geopm_ctl_c.3.gz
%doc %{_mandir}/man3/geopm_error.3.gz
%doc %{_mandir}/man3/geopm_fortran.3.gz
%doc %{_mandir}/man3/geopm_policy_c.3.gz
%doc %{_mandir}/man3/geopm_prof_c.3.gz
%doc %{_mandir}/man3/geopm_sched.3.gz
%doc %{_mandir}/man3/geopm_version.3.gz
%doc %{_mandir}/man7/geopm.7.gz

%files devel
%defattr(-,root,root,-)
%{_includedir}/geopm.h
%{_includedir}/geopm_ctl.h
%{_includedir}/geopm_error.h
%{_includedir}/geopm_policy.h
%{_includedir}/geopm_message.h
%{_includedir}/geopm_version.h
%{_includedir}/geopm_plugin.h

%files -n python-geopmpy
%{python_sitelib}/*
%{_bindir}/geopmanalysis
%{_bindir}/geopmaprun
%{_bindir}/geopmplotter
%{_bindir}/geopmsrun
%doc %{_mandir}/man1/geopmanalysis.1.gz
%doc %{_mandir}/man1/geopmaprun.1.gz
%doc %{_mandir}/man1/geopmplotter.1.gz
%doc %{_mandir}/man1/geopmsrun.1.gz
%doc %{_mandir}/man7/geopmpy.7.gz
%changelog
* Mon Jun 19 2017 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.3.0
- GEOPM alpha release!
- Modified implementations and interfaces:
- Added job launch wrapper script which simplifies GEOPM runtime launch.
- Added plotting support for visual analysis of report and trace data.
- Added python package: geopmpy for supporting python infrastructure (job launch/plotting).
- Added support for OMPT integration with the OpenMP runtime to mark GEOPM region entry and exit.
- Added support for PMPI interface use in fortran applications enabling full support for fortran applications.
- Added support to profile individual MPI functions as distinct regions.
- Added support for transmission of region hints from the application to the controller.
- Removed MPI_Pcontrol() interface for wrapping geopm_prof_*() interfaces.
- Removed geopm_ctl_spawn() interface.
- Removed geopm_prof_disable() interface.
- Changed to single aggregated report file per run instead of one per node.
- Changed the geopm_tprof_*() interfaces for thread progress.
- Changed GEOPM classes to derive from a pure virtual interface base class.
- Changed RPM build from RPM makefile in favor of geopm.spec.in/configure.
- Changed the report and trace file format to have headers with meta-data.
- Changed how the GEOPM_PROFILE environment variable is used: now dictates the profile name.
- Changed geopm_ctl_c interface to no longer be application facing.
- Changed requirement for power plane 0 controls: MSR no longer used/needed.
- Changed all application hints from *POLICY_HINT* to *REGION_HINT*.
- Changed build time wget/curl timeout periods to be longer.
- Updated features:
- Added support for per-cpu progress reporting from application.
- Added hint to ignore time spent in a region such that ignored region times are subtracted from epoch times.
- Added policy information to report.
- Added user id to shmkey prefix to avoid permissions issues with stale keys.
- Added man page for the geopmpy python package, geopmsrun and geopmaprun.
- Added documentation for new features and interface changes.
- Added cache file support to plotter.
- Added interface to Region object to get per-cpu progress.
- Added feature to track mpi runtime per region and print in the report.
- Added feature to treat unmarked code as a real region.
- Added support to resolve OMPT function address to a name in report.
- Added support launcher keeping controller off of Linux CPU 0 if possible.
- Added support for hyper-threads and multi socket system affinity support in launcher.
- Added significant rework of Environment class to avoid security issues.
- Added geopm_env_debug_attach() API.
- Added region hint support in the ModelRegion wrappers for integration tests.
- Added mvapich2 fortran90 test suite for testing GEOPM fortran interfaces.
- Added autotools make check support for python unit tests.
- Added standard PIP packaging of the geopmpy python package and posting on PYPI.
- Added build infrastructure for support for LLVM OpenMP runtime with OMPT enabled.
- Updated and extended integration tests:
- Added support for using launcher wrapper within integration tests.
- Added integration test for OMPT and MPI automatic region detection.
- Added better support for the integration test looping script.
- Added integration test job timeouts.
- Added proper clean up of reports when a test passes.
- Added setting of OMP_NUM_THREADS when running integration test.
- Added test to compare the regions detected in the trace to the report.
- Added integration test for MPI timing.
- Updated unit tests:
- Added unit tests for the Environment and SharedMemory classes.
- Added python unit test for affinity settings in the launcher script.
- Added support for edge cases in unit tests.
- Bug fixes:
- Fixed geopmpolicy to generate a whitelist file without requiring root.
- Fixed critical security issues from static analysis.
- Fixed missing symbol wrappers for init and finalize MPI fortran functions.
- Fixed buffer overflow in MPI API test.
- Fixed missing resize of m_level to the active number of levels per node in the TreeCommunicator.
- Fixed issue where gfortran does not support bit shift operations of more that 32 bits.
- Fixed shared memory cleanup at attach time.
- Fixed issue where PlatformImp was initialized twice.
- Fixed reporting of unmarked regions.
- Fixed bugs in plotter.
- Fixed const issue with MPI-2/MPI-3 interface definitions.
- Fixed big-o scaling for all2all ModelRegion.
- Fixed integration tests for unmarked regions.
- Fixed test_progress_exit integration test.
- Fixed standard directory specificiation in the spec file
- Fixed test_sample_rate integration test.
- Fixed check_run issue in scaling integration test.
- Fixed integration tests and unit tests to handle the new node-combined report with header format.
- Fixed launcher to check for srun affinity plugins before using them.
- Fixed fortran configure test for MPI-3 support.
- Fixed gfortran test to work with ubuntu.
- Fixed mac compile issues.
- Fixed fortran test makefile.
- Fixed documentation to remove all references to geopmkey.
* Wed Apr 05 2017 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.2.3
- Fixed broken OBS build of version 0.2.2.
- Fixed broken integration test for region timing.
* Tue Apr 04 2017 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.2.2
- Modified implementations and interfaces:
- Added environment variable GEOPM_RUN_LONG_TESTS to enable long running integration tests.
- Added environment variable GEOPM_KEEP_FILES to leave temporary files created by unit tests.
- Added environment variable GTEST_XML_DIR to configure location of junit xml output from unit tests.
- Changed documentation for geopm_epoch(): multiple calls per application is okay.
- Changed geopm_epoch() calls in examples to reflect new usage.
- Changed GoverningDecider to use much simpler and more effective algorithm.
- Changed all TreeCommunicator MPI runtime communication to send binary data: do not use MPI data marshaling.
- Changed all TreeCommunicator MPI runtime communication to one-sided MPI_Put() calls.
- Changed tuning for parameters used by BalancingDecider.
- Changed tuning for RAPL time window settings.
- Changed TDP percentage to double throughout code.
- Changed copyright dates for 2017.
- Updated features:
- Added least squared linear regression to calculate derivative.
- Added compiler optimizations for Intel when using Intel toolchain.
- Added environment control GEOPM_PROFILE_TIMEOUT of application timeout when waiting for controller.
- Added warning message about stale keys.
- Added throttling percentage to reports.
- Added GEOPM runtime/memory/network overhead calculation and reporting.
- Added --enable-overhead configure option for heavy-weight overhead measurement.
- Added support for Cray MPI.
- Added region IDs to report files.
- Added junit xml output from unit tests.
- Added energy hardware counter update sample triggering (reduce latency and jitter).
- Added memory buffering for trace object, buffer size is hardcoded to 128 MB (should be configurable).
- Added rpmbuild --nocheck support (check definition in spec file).
- Added minimal documentation about CPU affinity requirements.
- Added an example that will print affinity of MPI processes and OpenMP threads.
- Added a stability fix for power calculation that will be made more robust.
- Updated examples:
- Added CoMD to examples.
- Added QBOX to examples.
- Added AMG to examples.
- Updated and extended integration tests:
- Added support for ALPS to integration tests.
- Added support for resource manager detection.
- Added support for integration test environment configuration options.
- Added support for better signal handling to integration tests.
- Added integration tests that use the trace feature.
- Added integration tests for scaling compute node count.
- Added integration tests for power cap enforcement by GoverningDecider.
- Added integration tests that region entry is always preceded by region exit.
- Added integration tests for sample rate frequency and jitter.
- Added integration test for consistency between report and trace per region run-times.
- Updated unit tests:
- Added data driven unit test for derivative feature.
- Added unit tests for PMPI wrappers.
- Bug fixes:
- Fixed documentation for installing from OBS yum and zypper repos.
- Fixed some objects which were improperly using default copy constructor.
- Fixed issue where unmarked regions (region 0) would report a progress value other than zero.
- Fixed accounting issue when exiting a region and then immediately entering it again.
- Fixed issue where RAPL values would be reset upon PlatformImp destruction (bad behavior for applications that change values and exit like geopmpolicy).
- Fixed error handling in integration test script.
- Fixed issue due to changing return type of json_object_array_length() for different versions of the json-c library.
- Fixed issue preventing samples from being sent up tree beyond level 1.
- Fixed issue with stale shared memory keys by deleting them at start up.
- Fixed missing comm swap call in MPI_Gather() and MPI_Gatherv(): terminal error.
- Fixed TreeCommunicator topology mapping logic.
- Fixed issue with message vector sizing in TreeCommunicator.
- Fixed missing ronn executable documentation build issue.
- Fixed TreeCommunicator unit tests.
- Fixed MPIInterface tests exposed by CLANG.
- Fixed RAPL window MSR interface.
- Fixed user control of GNU standard build variables when running make.
- Fixed missing GEOPM annotation in some MPI wrappers in geopm_pmpi.c.
- Fixed accounting for region entries.
- Fixed issue by skipping TreeCommunicator tests on OpenMPI prior to 1.8.8 where one-sided comm was fixed.
* Fri Nov 18 2016 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.2.1
- Fix for accounting problem with nested MPI exits.
- Fix to thread calculation in integration test to avoid hyper-threads.
- Added script to loop over integration tests.
* Fri Nov 11 2016 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.2.0
- Renamed package to Global Extensible Open Power Manager.
- Improved features, performance, documentation, testing and continuous integration.
- Many bug fixes.
- Modified CONTRIBUTING.md to reflect current work-flow.
- Enabled Travis-CI on github repository.
- Linked Travis-CI to Open SUSE Build Service for automation of multi-distro packaging and testing.
- Removed explicit creation and destruction of geopm_prof_c objects from public interface.
- Introduced new environment variable GEOPM_PROFILE to control profiling.
- Introduced new environment variable GEOPM_DEBUG_ATTACH to enable attaching with a serial debugger.
- Removed geopm_prof_print interface.
- Removed "-r" command line option from geopmctl.
- Made the power budget in the policy an average per-node budget instead of a whole job budget.
- Modified report to include geopm version.
- Added accounting in report for the number of entries into each region.
- Added reporting of application totals.
- MPI is no longer explicitly a region and MPI accounting is now part of application totals.
- Refined how the geopm_prof_outer_sync() API works and renamed interface geopm_prof_epoch().
- The epoch start is no longer associated with application synchronization as geopm_prof_outer_sync was.
- Epoch start marks the beginning of the outer most iterative algorithm of the application.
- Added a --disable-doc configuration option for systems without ronn.
- Changed default shmem key base from "geopm_default" to "geopm-shm".
- Enabled GEOPM profiling without application modification through LD_PRELOAD.
- Appended domain numbers to the trace file column headers.
- Brought policy back to trace output.
- Modified implementation to print warning if controller is not found by the Profile interface.
- Enabled building in the SUSE environment.
- Added an example that prints the geopm hash of any string.
- Added support for Broadwell E Xeon and Knights Landing Xeon Phi platforms.
- Added capability to save/restore MSR values before/after GEOPM runs.
- Major improvements to signal handling and shutdown clean up.
- Improvements to temporary file and shared memory management.
- Added a suite of tutorials that steps through GEOPM features.
- Posted video walk through of the GEOPM tutorials to YouTube.
- Created the ideal "model" application for geopm shown in tutorial 6.
- Added integration test infrastructure using python unittest and model application.
- Added patches for GEOPM mark up to MiniFE and Nekbone benchmark source code.
- Added support for batch MSR read through msr-safe ioctl interface.
- Tuned decision making algorithms based on performance of several benchmarks.
- Allowed GoverningDecider to "unconverge."
- Added separate throttling times for sampling and control.
- Moved LockingHashTable template to a non-template implementation.
- Added distinct entries in profile table for MPI and epoch events.
- Switched to one sided communication (MPI_Put/MPI_Get) for passing samples up.
- When a new policy is received at the leaf it is enforced immediately.
- Modified implementation to unlink shared memory regions as soon as all users have attached.
- Added an example which will check if geopm supports the current platform which is used to skip some tests.
- Made check for supported platform more robust.
- Removed all throw calls inside destructor methods.
- Re-implemented application/controller handshake.
- Moved default profile object into Singleton pattern.
- Cleaned up factory registration pattern.
- Added better error checking of user inputs.
- Applied the write mask when writing to a MSR.
- Abstracted the read_bandwidth signal in the PlatformImp classes.
- Made PlatformImp objects abstract to signal topology.
- Added death tests for the controller.
- Removed use of MPI::Exception and all other MPI C++ constructs as they are deprecated.
- Wrote an abstraction of the hwloc interface remove hwloc version specific implementation requirement.
- Introduced XeonPlatformImp which Xeon platforms inherit from.
- Proposed a class interface to abstract MPI usage by GEOPM's controller.
- Fixed MSR read to mask off bits read from MSR beyond the overflow bit.
- Fixed possible under/over power budget conditions.
- Fixed a number of issues in report and trace output.
- Fixed issue where hash table could overflow.
- Fixed policy creation so that all the man page examples work correctly.
- Fixed subtraction of MPI time from outer sync time.
- Fixed accounting error in reported per region run-time.
- Fixed msr write logic for multi-socket systems.
- Fixed MSR save/restore.
- Fixed usage of RAPL time window 1 and 2.
- Fixed race condition: use MPI_Isend instead of MPI_Irsend.
- Fixed RAPL interface logic.
- Fixed geopm_time_add() to avoid overflowing nsec field.
- Fixed frequency calculation in report.
- Fixed the region entry count in report.
- Fixed issues around MPI_Request usage in non-blocking MPI calls.
- Fixed decider and accompanying logic.
- Fixed issue related to sending new polices down when new decisions are made.
- Fixed race condition in application/controller handshake.
- Fixed shutdown logic in PMPI wrapper when controller is run as a pthread.
- Fixed test executable so that non-matching test filters give an error.
- Fixed bug in MSR restore from file related to overflow.
- Fixed issue that occurs when using googlemock with gcc 6.
- Fixed issues around incorrect use of PMPI wrappers.
- Fixed a number of issues in the the PMPI wrappers.
- Fixed PMPI wrappers to work with both the MPI-2 and MPI-3 standards.
- Fixed missing dlclose() calls for dynamically opened shared objects.
- Fixed issue related to launching the controller with pthread in PMPI wrapper.
- Fixed multiple platform issues.
- Fixed death test issue due to inconsistent SLURM exit status codes.
- Fixed CPU indexing bug in PlatformImp derived classes.
- Fixed typo in Environment.cpp which was breaking GEOPM_ERROR_AFFINITY_IGNORE environment variable.
- Fixed the mask for getting frequency from IA32_PERF_STATUS.
- Fixed broken download, switched to Fedora URL for downloading gmock 1.7.0.
* Mon May 23 2016 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.1.1
- Fixed race condition in geopm_comm_split_shared().
- Fixed geopmctl so that it works properly (error introduced with policy environment).
- Fixed man page links and Makefile target.
- Fixed automatic detection of Fortran MPI flags for compile and other build fixes.
- Enable application marked with geopm_prof interface to run without controller.
- Better consistency checking in global policy.
- Enabled profile only use of geopm i.e. no power management (now the default).
- Updated STATUS section in README.
- Updated TODO list.
- Converted plugin developers guide to LaTeX and included it in repository.
* Mon May 09 2016 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.1.0
- First geopm release with code complete runtime component.
- Includes a wide range of bug fixes.
- Introduced Fortran interface for application APIs.
- Introduced globally scoped default profile object for geopm_prof_c interface.
- Introduced application tracing capability.
- Added NAS Fourier transform benchmark as an example.
- Fixes for build system.
- Fixes in the documentation.
- Remove thread profiling "helper APIs" and replace with geopm_tprof_c interface.
- Improvements in shutdown logic.
- Shared memory key has default value and can be obtained from environment.
- Explicit accounting for time spent in MPI calls through PMPI interface.
- Enable nesting of MPI regions within user defined regions.
- Remove geopm_prof_sample() interface.
- Add some helper APIs for splitting MPI communicators.
- Integrate with PMPI profiling interface to MPI.
- Merges irregular application feedback with periodic hardware telemetry.
- Moves some functionality between classes for better encapsulation.
- Region information is no longer communicated between compute nodes.
- Implemented plug-in selection through the Policy interface.
- Handling of MSR counter overflow.
- Implemented a basic decider for the leaf and the tree.
- Refactor of Platform/PlatformImp implementation.
- Updates to test infrastructure.
- Added a synthetic benchmark with static imbalance injection.
* Fri Dec 11 2015 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.0.3
- Several bug fixes.
- Update to user man pages.
- Switch to ronn for man page generation (roff + html).
- Major update to developer documentation with Doxygen.
- Implemented passing of profile data from application to controller.
- Implemented output of a summary profile report.
- Implemented infrastructure for plug-in extensions.
- Templatized CircularBuffer.
- Extended tests, including addition of integration tests.
* Fri Oct 16 2015 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.0.2
- Initial release to <https://github.com/geopm/geopm>.
- Updates to man pages.
- Support for static power modes.
- Support for Platform abstraction.
- Whitelist generation for MSR driver.
- TreeCommunicator implementation to support hierarchy in MPI.
- Build and test infrastructure (autotools, gtest, gmock).
* Thu Oct 1 2015 Christopher M. Cantalupo <christopher.m.cantalupo@intel.com> v0.0.1
- Initial tag which includes initial draft of man pages only.
