%define abi_ver 2.2.0
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

Name:		libgeopmd
Version:	3.2.0
Release:	%autorelease
Summary:	C/C++ implementation of the GEOPM access service

License:	BSD-3-Clause
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

BuildRequires:	autoconf
BuildRequires:	automake
BuildRequires:	gcc-c++
BuildRequires:	glibc-devel
BuildRequires:	gmock-devel
BuildRequires:	gtest-devel
BuildRequires:	libcap-devel
BuildRequires:	libtool
BuildRequires:	liburing-devel
BuildRequires:	systemd-devel
BuildRequires:	zlib-ng-compat-devel
BuildRequires:	grpc-devel
BuildRequires:	protobuf-devel

%if "%{_arch}" != "x86_64"
%define cpuid_option --disable-cpuid
%endif

%description
%{desc}

%package devel
Summary:	Development files for %{name}
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description devel
The %{name}-devel package contains libraries and header files for
applications that use %{name}.

%package -n geopmd-cli
Summary:	The libgeopmd command-line tools
Requires:	%{name}%{?_isa} = %{version}-%{release}

%description -n geopmd-cli
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
	--enable-grpc \
	%{?cpuid_option} \
	|| ( cat config.log && false )
%make_build
popd

%install
pushd %{name}
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
pushd %{name}
make check || (cat ./test-suite.log && false)
popd
%endif

%files
%license LICENSE-BSD-3-Clause
%doc CONTRIBUTING.rst README.md
%doc %{_docdir}/%{name}/LICENSE-BSD-3-Clause
%doc %{_docdir}/%{name}/VERSION
%{_libdir}/%{name}.so.%{abi_ver}
%{_libdir}/%{name}.so.2

%files devel
%{_includedir}/geopm
%{_includedir}/geopm_*
%{_libdir}/%{name}.so

%files -n geopmd-cli
%{_sbindir}/geopmbatch

%changelog
%autochangelog
