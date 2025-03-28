%define abi_ver 2.1.0
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

Name:		libgeopm2
Version:	3.2.0
Release:	%autorelease
Summary:	C/C++ implementation of the GEOPM runtime service

License:	BSD-3-Clause
Group:		System/Libraries
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gcc-c++
BuildRequires:	libtool
BuildRequires:	systemd-rpm-macros
BuildRequires:	libelf-devel
BuildRequires:	libgeopmd-devel
Recommends:	libgeopm-doc

%define docdir %{_defaultdocdir}/libgeopm

%description
%{desc}

%package -n libgeopm-devel
Summary:	Development files for %{name}
Group:		Development/Libraries/C and C++
Requires:	%{name} = %{version}-%{release}

%description -n libgeopm-devel
The libgeopm-devel package contains libraries and header files for
applications that use libgeopm.

%package -n geopm-cli
Summary:	libgeopm command-line tools
Group:		System/Management
Requires:	%{name} = %{version}-%{release}
Requires:	geopmd
Requires:	python3dist(geopmpy)

%description -n geopm-cli
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}

pushd libgeopm
echo %{version} > VERSION
autoreconf -vif
popd

%build
pushd libgeopm
%configure \
	--docdir=%{docdir} \
	--disable-mpi \
	--disable-openmp \
	--disable-fortran \
	--disable-geopmd-local
%make_build
popd

%install
pushd libgeopm
%make_install
rm -v %{buildroot}/%{_libdir}/libgeopm.a
rm -v %{buildroot}/%{_libdir}/libgeopm.la
rm -v %{buildroot}/%{_libdir}/geopm/libgeopmiogroup_profile.a
rm -v %{buildroot}/%{_libdir}/geopm/libgeopmiogroup_profile.la
rm -v %{buildroot}/%{_libdir}/geopm/libgeopmiogroup_profile.so
rm -v %{buildroot}/%{_libdir}/geopm/libgeopmiogroup_profile.so.2
%if "%{_bindir}" != "%{_sbindir}"
mkdir -p %{buildroot}%{_sbindir}
mv %{buildroot}{%{_bindir},%{_sbindir}}/geopmadmin
%endif
popd

%check
pushd libgeopm
make check || (cat ./test-suite.log && false)
popd

%post
ldconfig

%postun
ldconfig

%files
%dir %{docdir}
%doc %{docdir}/LICENSE-BSD-3-Clause
%doc %{docdir}/README.md
%doc %{docdir}/VERSION
%{_libdir}/libgeopm.so.%{abi_ver}
%{_libdir}/libgeopm.so.2
%{_libdir}/geopm
%{_libdir}/geopm/libgeopmiogroup_profile.so.%{abi_ver}

%files -n libgeopm-devel
%{_includedir}/geopm
%{_includedir}/geopm_*
%{_libdir}/libgeopm.so

%files -n geopm-cli
%{_sbindir}/geopmadmin
%{_bindir}/geopmagent
%{_bindir}/geopmctl

%if %{defined autochangelog}
%changelog
%autochangelog
%endif
