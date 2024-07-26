#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Packages: python3-geopmdpy

Summary: The geopmdpy Python package for the GEOPM Service
Name: python3-geopmdpy
Version: 3.1.0
Release: 1
License: BSD-3-Clause
URL: https://geopm.github.io
Source0: https://github.com/geopm/geopm/archive/v3.1.0/geopm-3.1.0.tar.gz
Patch0: 0001-Changes-required-for-building-from-git-archive.patch
Patch1: 0002-Fixup-TestActiveSessions-assertion.patch
Patch2: 0003-Fix-import-error-handling-for-setuptools_scm.patch
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: python3-devel
BuildRequires: python3-setuptools
BuildRequires: python-rpm-macros

# Packages required to run tests
BuildRequires: python3-dasbus >= 1.6
BuildRequires: python3-jsonschema
BuildRequires: python3-psutil
BuildRequires: python3-cffi
BuildRequires: libgeopmd2

%global debug_package %{nil}

%define python_bin %{__python3}

%{!?python3_pkgversion:%global python3_pkgversion 3}

%define python_major_version 3

Requires: libgeopmd2 = %{version}

Requires: python3-gobject-base
Requires: python3-dasbus >= 1.6
Requires: python3-jsonschema
Requires: python3-psutil
Requires: python3-cffi
Requires: libgeopmd2 = %{version}
Recommends: python3-geopmdpy-doc

%{?python_provide:%python_provide python%{python3_pkgversion}-geopmdpy}

%description

Python %{python_major_version} package for GEOPM service.  Provides
the implementation for the geopmd service daemon and interfaces for
configuring the service.

# Steps for all packages
%prep
%setup -n geopm-%{version}
%patch -P0 -p1
%patch -P1 -p1
%patch -P2 -p1
cd geopmdpy
echo %{version} > geopmdpy/VERSION

%build
cd geopmdpy
%py3_build

%check
cd geopmdpy
%{__python3} test

%install
cd geopmdpy
%py3_install

# Installed files
%files
%defattr(-,root,root,-)
%{python3_sitelib}/*
%{_bindir}/geopmd
%{_bindir}/geopmaccess
%{_bindir}/geopmsession

%changelog
* Fri May 17 2024 Christopher M Cantalupo <christopher.m.cantalupo@intel.com> v3.1.0
- Official v3.1.0 release tag
