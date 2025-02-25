%global debug_package %{nil}
%global prj_name geopmdpy
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

Name:		geopmd
Version:	3.2.0
Release:	%autorelease
Summary:	GEOPM daemon

License:	BSD-3-Clause
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

BuildRequires:	libgeopmd-devel
BuildRequires:	python3-devel
BuildRequires:	python3-setuptools
BuildRequires:	python3-setuptools_scm
BuildRequires:	python3-cffi
BuildRequires:	python3-dasbus
BuildRequires:	python3-jsonschema
BuildRequires:	python3-psutil
BuildRequires:	systemd-units
Requires:	python3-cffi
Requires:	python3-dasbus
Requires:	python3-jsonschema
Requires:	python3-psutil
Requires:	python3-%{prj_name} = %{version}-%{release}
Requires:	geopmd-cli
Requires:	python3-grpcio
Requires:	python3-protobuf

%description
%{desc}

%package -n python3-%{prj_name}
Summary:        Python bindings for libgeopmd

%description -n python3-%{prj_name}
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}
pushd %{prj_name}
echo %{version} > %{prj_name}/VERSION
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
python3 -m unittest discover -p 'Test*.py' -v %{_builddir}/%{prj_name}-%{version}/test
popd

%post -n geopmd
%systemd_post geopm.service

%preun -n geopmd
%systemd_preun geopm.service

%postun -n geopmd
%systemd_postun_with_restart geopm.service

%files
%license LICENSE-BSD-3-Clause
%doc README.md
%{_sbindir}/geopmd
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

%changelog
%autochangelog
