#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Packages: python3-geopmdpy
# This spec file requires the following variable be defined:
# 1. rpmbuild --define 'archive <PATH_TO_DISTRIBUTION_TARBALL>' ...

%global ver    %(python3 -c "from setuptools_scm import get_version; print(get_version('..'))")

Summary: The geomdpy package for the GEOPM Service
Name: python3-geopmdpy
Version: %{ver}
Release: 1
License: BSD-3-Clause
%if 0%{?rhel_version} || 0%{?centos_ver} || 0%{?rocky_ver}
# Deprecated for RHEL and CentOS
Group: System Environment/Libraries
%else
Group: Development/Libraries/Python
%endif
URL: https://geopm.github.io
Source0: %{archive}
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: python3-devel
BuildRequires: python3-setuptools

BuildRequires: python-rpm-macros

%define python_bin %{__python3}

%{!?python3_pkgversion:%global python3_pkgversion 3}

%define python_major_version 3

Requires: libgeopmd2 = %{version}

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

%description

Python %{python_major_version} package for GEOPM service.  Provides
the implementation for the geopmd service daemon and interfaces for
configuring the service.

# Steps for all packages
%prep
%setup -n geopmdpy-%{version}

%build
%py3_build

%install
%py3_install

# Installed files
%files
%defattr(-,root,root,-)
%{python3_sitelib}/*
%{_bindir}/geopmd
%{_bindir}/geopmaccess
%{_bindir}/geopmsession
