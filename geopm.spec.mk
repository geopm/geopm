#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

# targets for building rpm
version ?= 0.0.0
release ?= 1

rpm: geopm-$(version).tar.gz
	cp geopm-$(version).tar.gz geopm.tar.gz
	rpmbuild $(rpmbuild_flags) $^ -ta

geopm-$(version).spec:
	@echo "$$geopm_spec" > $@
	cat ChangeLog >> $@

.PHONY: rpm

define geopm_spec
Summary: Global Extensible Open Power Manager
Name: geopm
Version: $(version)
Release: $(release)
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
%if %{defined suse_version}
BuildRequires: libjson-c-devel
%else
BuildRequires: json-c-devel
%endif
%if 0%{?suse_version} >= 1320
BuildRequires: openssh
%endif

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

Development package for GEOPM.  Global Extensible Open Power Manager is
a hierarchical control system for optimizing power policy in a
power-constrained MPI job to increase performance.  It is designed to
coordinate power policy and performance across all of the compute nodes
hosting one MPI job on a portion of a distributed computing system.  The root
of the control hierarchy tree can communicate through shared memory with the
system resource management daemon to extend the hierarchy above the individual
MPI job level and enable management of system power resources for multiple MPI
jobs and multiple users by the system resource manager.  The geopm package
provides the libgeopm library the libgeopmpolicy library and the geopmctl
application.  The libgeopm library can be called within MPI applications to
enable application feedback for informing the control decisions.  If
modification of the target application is not desired then the geopmctl
application can be run concurrently with the target application.  In this
case, target application feedback is inferred by querying the hardware through
Model Specific Registers (MSRs).  With either method (libgeopm or geopmctl),
the control hierarchy tree writes processor power policy through MSRs to enact
policy decisions.  The libgeopmpolicy library is used by a resource manager to
set energy policy control parameters for MPI jobs.

%build
test -f configure || ./autogen.sh

%if %{defined suse_version}
./configure --prefix=%{_prefix} --libdir=%{_libdir} --libexecdir=%{_libexecdir} \
            --includedir=%{_includedir} --sbindir=%{_sbindir} \
            --mandir=%{_mandir} --docdir=%{docdir} \
            --with-mpi-bin=%{_libdir}/mpi/gcc/openmpi/bin \
            --disable-fortran --disable-doc
%else
./configure --prefix=%{_prefix} --libdir=%{_libdir} --libexecdir=%{_libexecdir} \
            --includedir=%{_includedir} --sbindir=%{_sbindir} \
            --mandir=%{_mandir} --docdir=%{docdir} \
            --with-mpi-bin=%{_libdir}/openmpi/bin \
            --disable-fortran --disable-doc
%endif

%{__make}

%check

%if %{defined suse_version}
MPIEXEC=/usr/lib64/mpi/gcc/openmpi/bin/mpiexec %{__make} check || \
( cat test/gtest_links/*.log && false )
%else
MPIEXEC=/usr/lib64/openmpi/bin/mpiexec %{__make} check || \
( cat test/gtest_links/*.log && false )
%endif

%install
%{__make} DESTDIR=%{buildroot} install
rm -f %{buildroot}/%{_libdir}/libgeopm.a
rm -f %{buildroot}/%{_libdir}/libgeopm.la
rm -f %{buildroot}/%{_libdir}/libgeopmpolicy.a
rm -f %{buildroot}/%{_libdir}/libgeopmpolicy.la
rm -f %{buildroot}/%{_libdir}/geopm/libgeopmpi_balancing.a
rm -f %{buildroot}/%{_libdir}/geopm/libgeopmpi_balancing.la
rm -f %{buildroot}/%{_libdir}/geopm/libgeopmpi_governing.a
rm -f %{buildroot}/%{_libdir}/geopm/libgeopmpi_governing.la

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
%dir %{_libexecdir}/geopm
%{_libexecdir}/geopm/geopm_launcher.py
%exclude %{_libexecdir}/geopm/geopm_launcher.pyc
%exclude %{_libexecdir}/geopm/geopm_launcher.pyo
%{_libexecdir}/geopm/geopm_plotter.py
%exclude %{_libexecdir}/geopm/geopm_plotter.pyc
%exclude %{_libexecdir}/geopm/geopm_plotter.pyo
%{_bindir}/geopmpolicy
%{_bindir}/geopmctl
%{_bindir}/geopmsrun
%{_bindir}/geopmaprun
%{_bindir}/geopmplotter
%dir %{docdir}
%doc %{docdir}/README
%doc %{docdir}/COPYING
%doc %{docdir}/VERSION
%doc %{_mandir}/man1/geopmctl.1.gz
%doc %{_mandir}/man1/geopmkey.1.gz
%doc %{_mandir}/man1/geopmpolicy.1.gz
%doc %{_mandir}/man1/geopm_launcher.1.gz
%doc %{_mandir}/man1/geopmplotter.1.gz
%doc %{_mandir}/man1/geopmaprun.1.gz
%doc %{_mandir}/man1/geopmsrun.1.gz
%doc %{_mandir}/man3/geopm_ctl_c.3.gz
%doc %{_mandir}/man3/geopm_error.3.gz
%doc %{_mandir}/man3/geopm_fortran.3.gz
%doc %{_mandir}/man3/geopm_omp.3.gz
%doc %{_mandir}/man3/geopm_policy_c.3.gz
%doc %{_mandir}/man3/geopm_prof_c.3.gz
%doc %{_mandir}/man3/geopm_version.3.gz
%doc %{_mandir}/man7/geopm.7.gz

%files devel
%defattr(-,root,root,-)
%{_includedir}/geopm.h
%{_includedir}/geopm_error.h
%{_includedir}/geopm_mpi_pcontrol.h
%{_includedir}/geopm_policy.h
%{_includedir}/geopm_message.h
%{_includedir}/geopm_version.h
%{_includedir}/geopm_plugin.h

%changelog
endef

export geopm_spec
