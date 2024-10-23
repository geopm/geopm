%define abi_ver 2.1.0
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

Name:		libgeopm
Version:	3.1.0
Release:	%autorelease
Summary:	C/C++ implementation of the GEOPM runtime service

License:	BSD-3-Clause
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

Patch0:		libgeopm-fedora.patch

ExclusiveArch:	x86_64

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gcc-c++
BuildRequires:	gmock-devel
BuildRequires:	gtest-devel
BuildRequires:	libtool
BuildRequires:	elfutils-libelf-devel
BuildRequires:	libgeopmd-devel

%description
%{desc}

%package devel
Summary:	Development files for %{name}
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description devel
The %{name}-devel package contains libraries and header files for
applications that use %{name}.

%package -n geopm-cli
Summary:	libgeopm command-line tools
Requires:	%{name}%{?_isa} = %{version}-%{release}
Requires:	geopmd
Requires:	python3dist(geopmpy)

%description -n geopm-cli
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}

pushd %{name}
echo %{version} > VERSION
autoreconf -vif
popd

%build
pushd %{name}
%configure \
	--disable-build-gtest \
	--disable-mpi \
	--disable-openmp \
	--disable-fortran \
	--disable-geopmd-local
%make_build
popd

%install
pushd %{name}
%make_install
rm -v %{buildroot}/%{_libdir}/libgeopm.a
popd

%check
pushd %{name}
make check
popd

%files
%license LICENSE-BSD-3-Clause
%doc CONTRIBUTING.rst README.md
%{_libdir}/%{name}.so.%{abi_ver}
%{_libdir}/%{name}.so.2

%files devel
%{_includedir}/geopm
%{_includedir}/geopm_*
%{_libdir}/%{name}.so

%files -n geopm-cli
%{_sbindir}/geopmadmin
%{_bindir}/geopmagent
%{_bindir}/geopmctl

%changelog
%autochangelog
