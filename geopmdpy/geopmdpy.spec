#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Packages: python3-geopmdpy
# This spec file requires the following variable be defined:
# 1. rpmbuild --define 'archive <PATH_TO_DISTRIBUTION_TARBALL>' ...

%global ver    %(python3 -c "from setuptools_scm import get_version; print(get_version('..'))")

Summary: Global Extensible Open Power Manager Service
Name: python-geopmdpy
Version: %{ver}
Release: 1
License: BSD-3-Clause
%if 0%{?rhel_version} || 0%{?centos_ver} || 0%{?rocky_ver}
# Deprecated for RHEL and CentOS
Group: System Environment/Daemons
%else
Group: System/Daemons
%endif
URL: https://geopm.github.io
Source0: %{archive}
BuildRoot: %{_tmppath}/geopmdpy-%{version}-%{release}-root
BuildRequires: python3-devel
BuildRequires: python3-setuptools

BuildRequires: systemd-rpm-macros
BuildRequires: python-rpm-macros

%define python_bin %{__python3}

%{!?python3_pkgversion:%global python3_pkgversion 3}

%define python_major_version 3

Requires: libgeopmd2 = %{version}

%package -n python%{python3_pkgversion}-geopmdpy

Summary: The geopmdpy Python package for the GEOPM Service
%if 0%{?rhel_version} || 0%{?centos_ver} || 0%{?rocky_ver}
# Deprecated for RHEL and CentOS
Group: System Environment/Libraries
%else
Group: Development/Libraries/Python
%endif

%if %{defined suse_version}
Requires: python3-gobject
BuildRequires: gobject-introspection
%else
Requires: python3-gobject-base
%endif
Requires: python3-dasbus >= 1.6
Requires: python3-jsonschema
Requires: python3-psutil
Requires: python3-cffi
Requires: libgeopmd2 = %{version}

%{?python_provide:%python_provide python%{python3_pkgversion}-geopmdpy}

%description -n python%{python3_pkgversion}-geopmdpy

Python %{python_major_version} package for GEOPM service.  Provides
the implementation for the geopmd service daemon and interfaces for
configuring the service.

# Steps for all packages
%global debug_package %{nil}
%prep
%setup

%generate_buildrequires
%pyproject_buildrequires

%build

%pyproject_wheel

%install
%pyproject_install
%pyproject_save_files %{srcname}

%clean

# Installed files

%files -n python%{python3_pkgversion}-geopmdpy -f %{pyproject_files}
%defattr(-,root,root,-)

