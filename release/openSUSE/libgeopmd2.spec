%define abi_ver 2.2.0
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

%if ! %{defined autorelease}
%define autorelease 1
%endif

Name:		libgeopmd2
Version:	3.2.0
Release:	%autorelease
Summary:	C/C++ implementation of the GEOPM access service

License:	BSD-3-Clause
Group:		System/Libraries
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gcc-c++
BuildRequires:	glibc-devel
BuildRequires:	gmock
BuildRequires:	gtest
BuildRequires:	libcap-devel
BuildRequires:	libtool
BuildRequires:	liburing-devel
BuildRequires:	systemd-devel
BuildRequires:	zlib-devel

# Lets GEOPM batch its IO operations to reduce syscall overhead in IOGroups
# with a lot of reads/writes per GEOPM batch operation.
%if %{defined disable_io_uring}
%define io_uring_option --disable-io-uring
%else
BuildRequires: liburing-devel
%endif

%if "%{_arch}" != "x86_64"
%define cpuid_option --disable-cpuid
%endif

# This spec file does not enable grpc by default.  Use the following syntax to
# enable grpc
#
#    rpmbuild --define 'enable_grpc 1' ...
%if %{defined enable_grpc}
%define grpc_option --enable-grpc
BuildRequires: grpc-devel
BuildRequires: protobuf-devel
%endif

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
%if %{defined enable_level_zero}
BuildRequires: level-zero
BuildRequires: level-zero-devel
Requires: level-zero >= 1.8.1
%endif
%if %{defined with_level_zero}
# Disable libze_loader as an explicit dependency for installing the RPM
%global __requires_exclude ^(libze_loader[.]so.*)$
%define level_zero_option --enable-levelzero --with-levelzero=%{with_level_zero}
%else
%if %{defined enable_level_zero}
%define level_zero_option --enable-levelzero
%endif
%endif

BuildRequires: fdupes

Recommends: libgeopmd-doc

%define docdir %{_defaultdocdir}/libgeopmd


%description
%{desc}

%package -n libgeopmd-devel
Summary:	Development files for %{name}
Group:		Development/Libraries/C and C++
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n libgeopmd-devel
The libgeopmd-devel package contains libraries and header files for
applications that use %{name}.

%package -n geopmd-cli
Summary:	The libgeopmd command-line tools
Group:		System/Management
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n geopmd-cli
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}

pushd libgeopmd
echo %{version} > VERSION
autoreconf -vif
popd

%build
pushd libgeopmd
%configure \
	--docdir=%{docdir} \
	--disable-build-gtest \
	%{?level_zero_option} \
	%{?io_uring_option} \
	%{?cpuid_option} \
	%{?grpc_option} \
	|| ( cat config.log && false )
%make_build
popd

%install
pushd libgeopmd
%make_install
rm -v %{buildroot}/%{_libdir}/libgeopmd.a
rm -v %{buildroot}/%{_libdir}/libgeopmd.la
%if "%{_bindir}" != "%{_sbindir}"
mkdir -p %{buildroot}%{_sbindir}
mv %{buildroot}{%{_bindir},%{_sbindir}}/geopmbatch
%endif
popd

%check
%if ! %{defined _without_check}
pushd libgeopmd
make check || (cat ./test-suite.log && false)
popd
%endif

%post
ldconfig

%postun
ldconfig

%files
%dir %{docdir}
%doc %{docdir}/LICENSE-BSD-3-Clause
%doc %{docdir}/README.md
%doc %{docdir}/VERSION
%{_libdir}/libgeopmd.so.%{abi_ver}
%{_libdir}/libgeopmd.so.2

%files -n libgeopmd-devel
%{_includedir}/geopm
%{_includedir}/geopm_*
%{_libdir}/libgeopmd.so

%files -n geopmd-cli
%{_sbindir}/geopmbatch


%if %{defined autochangelog}
%changelog
%autochangelog
%endif
