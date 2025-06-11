#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# This spec file does not enable grpc by default.  Use the following syntax to
# enable grpc
#
#    rpmbuild --define 'enable_grpc 1' ...


%global debug_package %{nil}
%global prj_name geopmdpy
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

Name:           geopmd
Version:	3.2.1
Release:	%autorelease
Summary:	GEOPM daemon

License:	BSD-3-Clause
Group:		System/Daemons
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

BuildRequires:	gobject-introspection
BuildRequires:	libgeopmd-devel = %{version}
BuildRequires:	python-gobject-common-devel
BuildRequires:	python3-cffi
BuildRequires:	python3-dasbus
BuildRequires:	python3-defusedxml
BuildRequires:	python3-devel
BuildRequires:	python3-gobject
BuildRequires:	python3-jsonschema
BuildRequires:	python3-psutil
BuildRequires:	python3-setuptools
BuildRequires:	python3-setuptools_scm
BuildRequires:	python3-sdnotify
%if %{defined enable_grpc}
BuildRequires:	grpc-devel
BuildRequires:	protobuf-devel
BuildRequires:	python3-grpcio
BuildRequires:	python3-protobuf
%endif
Requires:	python3-%{prj_name} = %{version}-%{release}
%description
%{desc}

%package -n python3-%{prj_name}
Summary:        Python bindings for libgeopmd
Group:		Development/Libraries/Python
Requires:	python3-cffi
Requires:	python3-dasbus
Requires:	python3-gobject
Requires:	python3-jsonschema
Requires:	python3-psutil
Requires:	python3-sdnotify
Requires:	python3-%{prj_name} = %{version}-%{release}
Requires:	geopmd-cli = %{version}
%if %{defined enable_grpc}
Requires:	python3-grpcio
Requires:	python3-protobuf
%endif
%description -n python3-%{prj_name}
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}
pushd %{prj_name}
echo %{version} > %{prj_name}/VERSION
%if %{defined enable_grpc}
./protoc-gen.sh
%endif
sed -i 's/usr\/bin/usr\/sbin/g' geopm.service
popd


%build
pushd %{prj_name}
%py3_build
popd

%install
pushd %{prj_name}
%py3_install
mkdir -p %{buildroot}%{_sysconfdir}/geopm
chmod 0700 %{buildroot}%{_sysconfdir}/geopm
%if "%{_bindir}" != "%{_sbindir}"
mkdir -p %{buildroot}%{_sbindir}
mv %{buildroot}{%{_bindir},%{_sbindir}}/geopmd
%endif
install -D -p -m 644 io.github.geopm.xml %{buildroot}%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
install -D -p -m 644 io.github.geopm.conf %{buildroot}%{_datadir}/dbus-1/system.d/io.github.geopm.conf
install -D -p -m 644 geopm.service %{buildroot}%{_unitdir}/geopm.service
popd

%check
pushd %{buildroot}%{python3_sitearch}
python3 -m unittest discover -p 'Test*.py' -v %{_builddir}/geopm-%{version}/%{prj_name}/test
popd

%pre -n geopmd
%service_add_pre geopm.service

%post -n geopmd
%service_add_post geopm.service

%preun -n geopmd
%service_del_preun geopm.service

%postun -n geopmd
%service_del_postun geopm.service

%files
%license LICENSE-BSD-3-Clause
%doc README.md
%{_sbindir}/geopmd
%dir %{_sysconfdir}/geopm
%dir %{_datadir}/dbus-1
%dir %{_datadir}/dbus-1/interfaces
%dir %{_datadir}/dbus-1/system.d
%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
%{_datadir}/dbus-1/system.d/io.github.geopm.conf
%{_unitdir}/geopm.service

%files -n python3-%{prj_name}
%{_bindir}/geopmaccess
%{_bindir}/geopmexporter
%{_bindir}/geopmread
%{_bindir}/geopmsession
%{_bindir}/geopmwrite
%{python3_sitearch}/%{prj_name}
%{python3_sitearch}/_libgeopmd_py_cffi.abi3.so
%{python3_sitearch}/%{prj_name}-*.egg-info

%if %{defined autochangelog}
%changelog
%autochangelog
%endif
