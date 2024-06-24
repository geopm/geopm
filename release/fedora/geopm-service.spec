#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Packages: geopm-service, geopm-service-devel, libgeopmd2
# This spec file supports three options for level-zero support:
#
# 1. default:
#    No level-zero support.
#
# 2. rpmbuild --define 'enable_level_zero 1' ...
#    Enable level-zero with packages installed into standard
#    locations: libdir and includedir.
#
# 3. rpmbuild --define 'with_level_zero <LEVEL_ZERO_PREFIX>' ...
#    Enable level-zero with packages installed into a
#    non-standard prefix.

Summary: Global Extensible Open Power Manager Service
Name: geopm-service
Version: 3.1.0
Release: 1
License: BSD-3-Clause
URL: https://geopm.github.io
Source0: https://github.com/geopm/geopm/archive/v3.1.0/geopm-3.1.0.tar.gz
Patch0: 0001-Changes-required-for-building-from-git-archive.patch
BuildRoot: %{_tmppath}/geopm-service-%{version}-%{release}-root
Prefix: %{_prefix}
BuildRequires: gcc-c++
BuildRequires: unzip
BuildRequires: libtool
BuildRequires: libcap-devel
BuildRequires: systemd-devel >= 221
BuildRequires: zlib-devel
BuildRequires: gtest-devel >= 1.12.1
BuildRequires: gmock-devel >= 1.12.1
%if %{defined enable_level_zero}
BuildRequires: level-zero
BuildRequires: level-zero-devel
Requires: level-zero >= 1.8.1
%endif
BuildRequires: systemd-rpm-macros
Recommends: geopm-service-doc

%define docdir %{_defaultdocdir}/geopm-service-%{version}

Requires: libgeopmd2 = %{version}
Requires: python3-geopmdpy = %{version}

%description

The GEOPM Service provides a user-level interface to read telemetry
and configure settings of heterogeneous hardware platforms. Linux
system administrators may manage permissions for user access to
telemetry and configuration at a fine granularity.  This package
includes the geopm systemd service unit that provides a DBus interface
io.github.geopm.  Additionally the libgeopmd.so shared object library
is installed with this package.


%package devel

Summary: Global Extensible Open Power Manager Service - development
Requires: libgeopmd2 = %{version}

%description devel

Development package for the GEOPM Service.  This provides the
programming interface to libgeopmd.so.  The package includes the C and
C++ header files, maunuals for these interfaces and the unversioned
libgeopmd.so shared object symbolic link.

%package -n libgeopmd2

Summary: Provides libgeopmd shared object library
Recommends: libgeopmd-doc

# Lets GEOPM batch its IO operations to reduce syscall overhead in IOGroups
# with a lot of reads/writes per GEOPM batch operation.
%if %{defined disable_io_uring} || 0%{?centos_ver} == 8
%define io_uring_option --disable-io-uring
%else
BuildRequires: liburing-devel
%endif

%if "%{_arch}" != "x86_64"
%define cpuid_option --disable-cpuid
%endif

%description -n libgeopmd2

Library supporting the GEOPM Service.  This provides the libgeopmd
library which provides C and C++ interfaces.

# Steps for all packages
%prep
%setup -n geopm-%{version}
%patch -P0 -p1

%build
cd libgeopmd

%if %{defined with_level_zero}
# Disable libze_loader as an explicit dependency for installing the RPM
%global __requires_exclude ^(libze_loader[.]so.*)$
%define level_zero_option --enable-levelzero --with-levelzero=%{with_level_zero}
%else
%if %{defined enable_level_zero}
%define level_zero_option --enable-levelzero
%endif
%endif
%if 0%{?fedora} != 0 && ! 0%{?fedora} <= 40
# Our use of relro flag causes a configure test to fail on fedora rawhide
%define relro_flags LDFLAGS=-Wl,-z,norelro
%endif

echo %{version} > VERSION
./autogen.sh
%{?relro_flags} \
./configure --prefix=%{_prefix} --libdir=%{_libdir} --libexecdir=%{_libexecdir} \
            --includedir=%{_includedir} --sbindir=%{_sbindir} \
            --mandir=%{_mandir} --docdir=%{docdir} \
            --disable-build-gtest \
            %{?level_zero_option} \
            %{?io_uring_option} \
            %{?cpuid_option} \
            || ( cat config.log && false )

%{__make} %{?_smp_mflags}

%check
cd libgeopmd
CFLAGS= CXXFLAGS= CC=gcc CXX=g++ \
%{__make} %{?_smp_mflags} check

%install
cd libgeopmd
%{__make} DESTDIR=%{buildroot} install
rm -f $(find %{buildroot}/%{_libdir} -name '*.a'; \
        find %{buildroot}/%{_libdir} -name '*.la')

install -Dp -m644 geopm.service %{buildroot}%{_unitdir}/geopm.service
install -Dp -m644 io.github.geopm.conf %{buildroot}%{_datadir}/dbus-1/system.d/io.github.geopm.conf
install -Dp -m644 io.github.geopm.xml %{buildroot}%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
mkdir -p %{buildroot}%{_sysconfdir}/geopm
mkdir -p %{buildroot}%{_sbindir}
ln -s -r %{buildroot}%{_sbindir}/service %{buildroot}%{_sbindir}/rcgeopm

%clean

%post -n geopm-service
cd libgeopmd
%systemd_post geopm.service

%preun -n geopm-service
cd libgeopmd
%systemd_preun geopm.service

%postun -n geopm-service
cd libgeopmd
%systemd_postun_with_restart geopm.service

# Installed files

%files
%defattr(-,root,root,-)
%{_sbindir}/rcgeopm
%{_bindir}/geopmread
%{_bindir}/geopmwrite
%dir %{_datadir}/dbus-1
%dir %{_datadir}/dbus-1/system.d
%{_datadir}/dbus-1/system.d/io.github.geopm.conf
%dir %{_datadir}/dbus-1/interfaces
%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
%{_unitdir}/geopm.service
%dir %{docdir}
%doc %{docdir}/LICENSE-BSD-3-Clause
%doc %{docdir}/README.md
%doc %{docdir}/VERSION

%files -n libgeopmd2
%defattr(-,root,root,-)
%{_libdir}/libgeopmd.so.2.1.0
%{_libdir}/libgeopmd.so.2
%dir %{_libdir}/geopm

%files devel
%defattr(-,root,root,-)
%dir %{_includedir}/geopm
%{_includedir}/geopm/Agg.hpp
%{_includedir}/geopm/Cpuid.hpp
%{_includedir}/geopm/CircularBuffer.hpp
%{_includedir}/geopm/Exception.hpp
%{_includedir}/geopm/Helper.hpp
%{_includedir}/geopm/IOGroup.hpp
%{_includedir}/geopm/json11.hpp
%{_includedir}/geopm/PlatformIO.hpp
%{_includedir}/geopm/PlatformTopo.hpp
%{_includedir}/geopm/PluginFactory.hpp
%{_includedir}/geopm/SaveControl.hpp
%{_includedir}/geopm/ServiceProxy.hpp
%{_includedir}/geopm/SharedMemory.hpp
%{_includedir}/geopm/SharedMemoryScopedLock.hpp
%{_includedir}/geopm_debug.hpp
%{_includedir}/geopm_error.h
%{_includedir}/geopm_field.h
%{_includedir}/geopm_hash.h
%{_includedir}/geopm_hint.h
%{_includedir}/geopm_pio.h
%{_includedir}/geopm_plugin.hpp
%{_includedir}/geopm_public.h
%{_includedir}/geopm_sched.h
%{_includedir}/geopm_shmem.h
%{_includedir}/geopm_time.h
%{_includedir}/geopm_topo.h
%{_includedir}/geopm_version.h
%{_libdir}/libgeopmd.so

%changelog
* Fri May 17 2024 Christopher M Cantalupo <christopher.m.cantalupo@intel.com> v3.1.0
- Official v3.1.0 release tag
