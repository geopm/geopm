%global debug_package %{nil}
%global prj_name geopmdpy
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

Name:           geopmd
Version:	3.1.0
Release:	%autorelease
Summary:	GEOPM daemon

License:	BSD-3-Clause
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz

Patch0:		TestActiveSessions-assertion.patch

ExclusiveArch:	x86_64

BuildRequires:	libgeopmd-devel
BuildRequires:	python3-devel
BuildRequires:	python3-setuptools
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
Requires:	geopm-cli

%description
%{desc}

%package -n python3-%{prj_name}
Summary:        Python bindings for libgeopmd

%description -n python3-%{prj_name}
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}
sed -i 's/usr\/bin/usr\/sbin/g' libgeopmd/geopm.service

pushd %{prj_name}
echo %{version} > %{prj_name}/VERSION
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
mkdir -p %{buildroot}%{_sbindir}
mv %{buildroot}{%{_bindir},%{_sbindir}}/geopmd
popd
install -D -p -m 644 libgeopmd/io.github.geopm.xml %{buildroot}%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
install -D -p -m 644 libgeopmd/io.github.geopm.conf %{buildroot}%{_datadir}/dbus-1/system.d/io.github.geopm.conf
install -D -p -m 644 libgeopmd/geopm.service %{buildroot}%{_unitdir}/geopm.service

%check
pushd %{prj_name}
%{python3} -m unittest discover -p 'Test*.py' -v
popd

%files
%license LICENSE-BSD-3-Clause
%doc README.md
%{_bindir}/geopmaccess
%{_bindir}/geopmsession
%{_sbindir}/geopmd
%dir %{_sysconfdir}/geopm
%{_datadir}/dbus-1/interfaces/io.github.geopm.xml
%{_datadir}/dbus-1/system.d/io.github.geopm.conf
%{_unitdir}/geopm.service

%files -n python3-%{prj_name}
%{python3_sitelib}/%{prj_name}
%{python3_sitelib}/%{prj_name}-*.egg-info

%changelog
%autochangelog
