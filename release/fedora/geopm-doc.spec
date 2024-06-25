#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Packages: geopm-service, geopm-service-devel, libgeopmd2
# This spec file supports three options for level-zero support:
#
# 1. default:
#    No level-zero support.
#
# 2. rpmbuild --define 'enable_level_zero 1' ...
#    Enable level-zero with packages installed into standard
#    locations: libdir and includedir.
#
# 3. rpmbuild --define 'with_level_zero <LEVEL_ZERO_PREFIX>' ...
#    Enable level-zero with packages installed into a
#    non-standard prefix.

Summary: Global Extensible Open Power Manager Documentation
Name: geopm-doc
Version: 3.1.0
Release: 1
License: BSD-3-Clause
URL: https://geopm.github.io
Source0: https://github.com/geopm/geopm/archive/v3.1.0/geopm-3.1.0.tar.gz
Patch0: 0001-Changes-required-for-building-from-git-archive.patch
BuildRoot: %{_tmppath}/geopm-doc-%{version}-%{release}-root
Prefix: %{_prefix}
BuildRequires: python3-sphinx >= 4.5.0
BuildRequires: python3-sphinx_rtd_theme >= 1.0.0
BuildRequires: python3-sphinxemoji >= 0.2.0
BuildRequires: python3-pygments >= 2.13.0
BuildRequires: python3-sphinx-tabs >= 3.3.1
BuildRequires: doxygen
BuildRequires: graphviz
Requires: geopm-service-doc
Requires: libgeopmd-doc
Requires: python3-geopmdpy-doc
Requires: geopm-runtime-doc
Requires: libgeopm-doc
Requires: python3-geopmpy-doc
BuildRequires: bash-completion
BuildRequires: marshalparser
%global debug_package %{nil}

%define completionsdir %(pkg-config --variable=completionsdir bash-completion)
%if "x%{?completionsdir}" == "x"
%define completionsdir "%{_sysconfdir}/bash_completion.d"
%endif

%description
Man pages for geopm packages

%package  -n geopm-runtime-doc
Summary: Global Extensible Open Power Manager Runtime Documentation
Group: Documentation
%description -n geopm-runtime-doc
Man pages for geopm-runtime package

%package  -n geopm-service-doc
Summary: Global Extensible Open Power Manager Service Documentation
Group: Documentation
%description -n geopm-service-doc
Man pages for geopm-service package

%package  -n libgeopmd-doc
Summary: Global Extensible Open Power Manager Service Library Documentation
Group: Documentation
%description -n libgeopmd-doc
Man pages for libgeopmd package

%package  -n libgeopm-doc
Summary: Global Extensible Open Power Manager Runtime Library Documentation
Group: Documentation
%description -n libgeopm-doc
Man pages for libgeopm package

%package  -n python3-geopmdpy-doc
Summary: Global Extensible Open Power Manager Service Python Documentation
Group: Documentation
%description -n python3-geopmdpy-doc
Man pages for python3-geopmdpy package

%package  -n python3-geopmpy-doc
Summary: Global Extensible Open Power Manager Runtime Python Documentation
Group: Documentation
%description -n python3-geopmpy-doc
Man pages for python3-geopmpy package

%prep
%setup -n geopm-%{version}
%patch -P0 -p1

%build
cd docs
echo %{version} > VERSION
%{__make} man

%install
cd docs
%{__make} DESTDIR=%{buildroot} prefix=%{_prefix} datarootdir=%{_datarootdir} mandir=%{_mandir} install_man
%if 0%{?fedora} <= 40
%{__make} DESTDIR=%{buildroot} prefix=%{_prefix} datarootdir=%{_datarootdir} mandir=%{_mandir} completionsdir=%{completionsdir} install_completion
%endif

%clean

# Installed files
%files

%files -n geopm-runtime-doc
%defattr(-,root,root,-)
%doc %{_mandir}/man1/geopmadmin.1.gz
%doc %{_mandir}/man1/geopmagent.1.gz
%doc %{_mandir}/man1/geopmctl.1.gz
%doc %{_mandir}/man7/geopm_agent_ffnet.7.gz
%doc %{_mandir}/man7/geopm_agent_frequency_map.7.gz
%doc %{_mandir}/man7/geopm_agent_monitor.7.gz
%doc %{_mandir}/man7/geopm_agent_power_balancer.7.gz
%doc %{_mandir}/man7/geopm_agent_power_governor.7.gz

%files -n geopm-service-doc
%defattr(-,root,root,-)
%doc %{_mandir}/man1/geopmread.1.gz
%doc %{_mandir}/man1/geopmwrite.1.gz
%doc %{_mandir}/man7/geopm.7.gz
%doc %{_mandir}/man7/geopm_pio.7.gz
%doc %{_mandir}/man7/geopm_pio_cnl.7.gz
%doc %{_mandir}/man7/geopm_pio_const_config.7.gz
%doc %{_mandir}/man7/geopm_pio_cpuinfo.7.gz
%doc %{_mandir}/man7/geopm_pio_dcgm.7.gz
%doc %{_mandir}/man7/geopm_pio_levelzero.7.gz
%doc %{_mandir}/man7/geopm_pio_msr.7.gz
%doc %{_mandir}/man7/geopm_pio_nvml.7.gz
%doc %{_mandir}/man7/geopm_pio_profile.7.gz
%doc %{_mandir}/man7/geopm_pio_service.7.gz
%doc %{_mandir}/man7/geopm_pio_sst.7.gz
%doc %{_mandir}/man7/geopm_pio_sysfs.7.gz
%doc %{_mandir}/man7/geopm_pio_time.7.gz
%doc %{_mandir}/man7/geopm_report.7.gz
%if 0%{?fedora} <= 40
%doc %{completionsdir}/geopmread
%doc %{completionsdir}/geopmwrite
%endif

%files -n libgeopmd-doc
%doc %{_mandir}/man3/geopm::Agg.3.gz
%doc %{_mandir}/man3/geopm::CNLIOGroup.3.gz
%doc %{_mandir}/man3/geopm::CircularBuffer.3.gz
%doc %{_mandir}/man3/geopm::CpuinfoIOGroup.3.gz
%doc %{_mandir}/man3/geopm::Exception.3.gz
%doc %{_mandir}/man3/geopm::Helper.3.gz
%doc %{_mandir}/man3/geopm::IOGroup.3.gz
%doc %{_mandir}/man3/geopm::MSRIO.3.gz
%doc %{_mandir}/man3/geopm::MSRIOGroup.3.gz
%doc %{_mandir}/man3/geopm::PlatformIO.3.gz
%doc %{_mandir}/man3/geopm::PlatformTopo.3.gz
%doc %{_mandir}/man3/geopm::PluginFactory.3.gz
%doc %{_mandir}/man3/geopm::SampleAggregator.3.gz
%doc %{_mandir}/man3/geopm::SharedMemory.3.gz
%doc %{_mandir}/man3/geopm::TimeIOGroup.3.gz
%doc %{_mandir}/man3/geopm_error.3.gz
%doc %{_mandir}/man3/geopm_field.3.gz
%doc %{_mandir}/man3/geopm_hash.3.gz
%doc %{_mandir}/man3/geopm_pio.3.gz
%doc %{_mandir}/man3/geopm_sched.3.gz
%doc %{_mandir}/man3/geopm_time.3.gz
%doc %{_mandir}/man3/geopm_topo.3.gz
%doc %{_mandir}/man3/geopm_version.3.gz

%files -n libgeopm-doc
%doc %{_mandir}/man3/geopm::Agent.3.gz
%doc %{_mandir}/man3/geopm::CPUActivityAgent.3.gz
%doc %{_mandir}/man3/geopm::PowerBalancer.3.gz
%doc %{_mandir}/man3/geopm::PowerGovernor.3.gz
%doc %{_mandir}/man3/geopm_agent.3.gz
%doc %{_mandir}/man3/geopm_ctl.3.gz
%doc %{_mandir}/man3/geopm_imbalancer.3.gz
%doc %{_mandir}/man3/geopm_prof.3.gz

%files -n python3-geopmdpy-doc
%doc %{_mandir}/man1/geopmaccess.1.gz
%doc %{_mandir}/man1/geopmsession.1.gz
%doc %{_mandir}/man7/geopmdpy.7.gz

%files -n python3-geopmpy-doc
%doc %{_mandir}/man1/geopmlaunch.1.gz
%doc %{_mandir}/man7/geopmpy.7.gz
%if 0%{?fedora} <= 40
%doc %{completionsdir}/geopmlaunch
%endif

%changelog
* Fri May 17 2024 Christopher M Cantalupo <christopher.m.cantalupo@intel.com> v3.1.0
- Official v3.1.0 release tag
