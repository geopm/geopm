%if ! 0%{?fedora}
%define _without_check 1
%endif
%global debug_package %{nil}
%global prj_name geopmpy
%global desc %{expand: \
The Global Extensible Open Power Manager (GEOPM) provides a framework to
explore power and energy optimizations on platforms with heterogeneous mixes
of computing hardware.

Users can monitor their system's energy and power consumption, and safely
optimize system hardware settings to achieve energy efficiency and/or
performance objectives.}

Name:		python-%{prj_name}
Version:	3.2.0
Release:	%autorelease
Summary:	Python bindings for libgeopm

License:	BSD-3-Clause
URL:		https://geopm.github.io
Source0:	https://github.com/geopm/geopm/archive/v%{version}/geopm-%{version}.tar.gz
Patch0:		0002-Allow-numpy-2.0-and-higher.patch

BuildRequires:	gcc
BuildRequires:	python3-cffi
BuildRequires:	python3-devel
BuildRequires:	python3-setuptools
BuildRequires:	python3-setuptools_scm
BuildRequires:	python3-geopmdpy
BuildRequires:	libgeopm-devel
BuildRequires:	libgeopmd-devel
%if ! %{defined _without_check}
BuildRequires:	python3-pandas
BuildRequires:	python3-natsort
BuildRequires:	python3-psutil
BuildRequires:	python3-pyyaml
BuildRequires:	python3-tables
%endif
Requires:	geopmd

%description
%{desc}

%package -n python3-%{prj_name}
Summary:	%{summary}

%description -n python3-%{prj_name}
%{desc}

%prep
%autosetup -p1 -n geopm-%{version}

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
popd

%check
%if ! %{defined _without_check}
pushd %{buildroot}%{python3_sitearch}
python3 -m unittest discover -p 'Test*.py' -v %{_builddir}/geopm-%{version}/%{prj_name}/test
popd
%endif

%files -n python3-%{prj_name}
%license LICENSE-BSD-3-Clause
%doc README.md
%{_bindir}/geopmlaunch
%{python3_sitearch}/%{prj_name}
%{python3_sitearch}/_libgeopm_py_cffi.abi3.so
%{python3_sitearch}/%{prj_name}-*.egg-info

%changelog
%autochangelog
