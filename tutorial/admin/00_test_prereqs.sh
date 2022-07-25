#!/bin/bash
#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -o pipefail

RC=0

# Helper funcs
command_available_and_executable(){
    local CMD=${1}
    local LRC=0
    if [ ! -x "$(command -v ${CMD})" ]; then
        echo "ERROR: Required command '${CMD}' not available."
        LRC=1
        RC=1 # Global RC for script status
    fi
    return ${LRC}
}

# GEOPM installed and executable
command_available_and_executable geopmadmin
command_available_and_executable geopmagent
command_available_and_executable geopmbench
command_available_and_executable geopmctl
command_available_and_executable geopmlaunch
command_available_and_executable geopmread
command_available_and_executable geopmwrite

# Check GNU vs. Intel Toolchain
if ! ldd $(which geopmbench) | grep --quiet libimf; then
    echo "WARNING: Please build geopm with the Intel Toolchain"
    echo "         to ensure the best performance."
fi

# Check for missing libs
if ldd $(which geopmctl) | grep --quiet "not found"; then
    echo "ERROR: The following runtime libraries are missing:"
    ldd $(which geopmctl) | grep "not found"
    RC=1
fi

# Python reqs
PYTHON_CMD=python3
if ! 2>/dev/null ${PYTHON_CMD} -c "import geopmpy"; then
    echo "ERROR: geopmpy is not in the PYTHONPATH."
    echo "       Please read the \"USER ENVIRONMENT\" section of the main README."
    RC=1
elif ! 2>/dev/null ${PYTHON_CMD} -c "import geopmpy.launcher"; then
    echo "ERROR: The following Python dependencies are missing:"
    2>&1 ${PYTHON_CMD} -c "import geopmpy.launcher" | grep Module
    echo "       Please read the \"PYTHON INSTALL\" section of the main README."
    RC=1
fi

# SYSTEMD Config check
if [ "$(basename $(realpath /sbin/init))" = "systemd" ]; then
    # Using SYSTEMD
    CONF=/etc/systemd/logind.conf
    if [ "$(grep --quiet ^RemoveIPC ${CONF} | cut -d= -f2)" = "yes" ]; then
        echo "ERROR: systemd is set improperly and will cause shared memory"
        echo "       issues with GEOPM.  Set RemoveIPC=no in ${CONF}"
        echo "       Please read the \"SYSTEMD CONFIGURATION\" section of the main README."
        RC=1
    elif [ "$(grep --quiet ^#RemoveIPC ${CONF} | cut -d= -f2)" = "yes" ]; then
        echo "ERROR: The compile time defaults for systemd will cause issues"
        echo "       with GEOPM.  Set RemoveIPC=no in ${CONF}"
        echo "       Please read the \"SYSTEMD CONFIGURATION\" section of the main README."
        RC=1
    elif ! grep --quiet RemoveIPC ${CONF}; then
        echo "WARNING: Unable to determine RemoveIPC value from ${CONF}"
        echo "         Run the GEOPM unit tests to ensure there are no issues"
        echo "         with shared memory.  Specifically: SharedMemoryTest.fd_check"
        echo "         Please read the \"SYSTEMD CONFIGURATION\" section of the main README."
    fi
else
    echo "WARNING: Your system is not using systemd.  Please run the GEOPM
                   unit tests to ensure there are no issues with shared memory.
                   Specifically: SharedMemoryTest.fd_check"
fi

# msr-safe configuration
MSRSAVE=/usr/sbin/msrsave
if command_available_and_executable ${MSRSAVE}; then
    REQUIRED_VERSION=1.4.0
    INSTALLED_VERSION=$(${MSRSAVE} --version | head -n1 | cut -d- -f1)
    if [ "${REQUIRED_VERSION}" != "$(echo -e "${REQUIRED_VERSION}\n${INSTALLED_VERSION}" | sort -V | head -n1)" ]; then
        echo "ERROR: Installed version of msr-safe (${INSTALLED_VERSION}) is too old."
        echo "       Please update msr-safe to at least version ${REQUIRED_VERSION}."
        RC=1
    fi
else
    echo "WARNING: Unable to find/execute ${MSRSAVE}.  Unable to determine msr-safe version."
fi

FILE=/dev/cpu/msr_batch
if [ ! -c ${FILE} ]; then
    echo "ERROR: ${FILE} is not available.  Please install msr-safe."
    RC=1
fi

TMPFILE=$(mktemp --tmpdir msrsave_test.XXXXXXXXXX)
if ! ${MSRSAVE} ${TMPFILE}; then
    echo "ERROR: ${MSRSAVE} had a non-zero return code.  Please contact"
    echo "       The GEOPM or msr-safe maintainers for more information."
    RC=1
fi
rm ${TMPFILE}

./00_check_allowlist.sh # GEOPM required MSRs set properly in the msr-safe allowlist
if [ $? -eq 1 ]; then
    RC=1
fi

# MSR specific checks
if [ $(geopmread MSR::PKG_POWER_LIMIT:LOCK package 0) -eq 1 ]; then
    echo "WARNING: The lock bit for the PKG_POWER_LIMIT MSR is set.  The power_balancer"
    echo "         and power_governor agents will not function properly until this is cleared."
else
    # Can my user set a MSR value?
    CURRENT_PWR_LIMIT=$(geopmread MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT package 0)
    if ! geopmwrite MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT package 0 $(( ${CURRENT_PWR_LIMIT} - 1 )); then
        echo "ERROR: Unable to set PKG_POWER_LIMIT.  The power_balancer"
        echo "       agent will not function properly."
        RC=1
    else
        geopmwrite MSR::PKG_POWER_LIMIT:PL1_POWER_LIMIT package 0 ${CURRENT_PWR_LIMIT}
    fi
fi

CURRENT_FREQ=$(geopmread MSR::PERF_CTL:FREQ package 0)
STICKER_FREQ=$(geopmread CPU_FREQUENCY_STICKER board 0)
TEST_FREQ="$(( ${STICKER_FREQ} - $(geopmread CPU_FREQUENCY_STEP board 0) ))"
if ! geopmwrite MSR::PERF_CTL:FREQ package 0 ${TEST_FREQ}; then
    echo "ERROR: Unable to set frequency with geopmwrite."
    RC=1
else
    CURRENT_FREQ=$(geopmread MSR::PERF_CTL:FREQ package 0)
    if [ "${CURRENT_FREQ}" != "${TEST_FREQ}" ]; then
        echo "ERROR: Current frequency (${CURRENT_FREQ}) != test frequency (${TEST_FREQ})."
        RC=1
    fi
    geopmwrite MSR::PERF_CTL:FREQ package 0 ${CURRENT_FREQ}
fi

# OS Power Management
if ! grep --quiet intel_pstate=disable /proc/cmdline; then
    echo "ERROR: The intel_pstate CPU scaling driver was not explicitly disabled"
    echo "       on the kernel commandline.  This will cause issues with GEOPM's"
    echo "       ability to control frequency."
    RC=1
fi

SCALING_DRIVER=/sys/devices/system/cpu/cpu0/cpufreq/scaling_driver
if [ "$(cat ${SCALING_DRIVER})" = "intel_pstate" ]; then
    echo "ERROR: intel_pstate was detected as the CPU scaling driver."
    echo "       This will cause issues with GEOPM's ability to control frequency."
    RC=1
fi
SCALING_GOVERNOR=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor
if [ "$(cat ${SCALING_GOVERNOR})" = "userspace" ]; then
    echo "WARNING: The CPU scaling driver is userspace.  This may limit GEOPM's"
    echo "         ability to control frequency.  Please examine the contents of"
    echo "         scaling_setspeed in the CPU-device directory."
elif [ "$(cat ${SCALING_GOVERNOR})" != "performance" ]; then
    echo "ERROR: The CPU scaling governor was not userspace or performance."
    echo "       This will cause issues with GEOPM's ability to control frequency."
    RC=1
fi

IDLE_DRIVER=/sys/devices/system/cpu/cpuidle/current_driver
if [ "$(cat ${IDLE_DRIVER})" = "acpi_idle" ]; then
    echo "WARNING: The acpi_idle driver was detected.  You may improve"
    echo "         idle load node efficiency by switching to the intel_idle driver."
fi

# Software that may interfere with GEOPM
if [ ! -z "${SLURM_EAR_LIBRARY}" ] && [ "${SLURM_EAR_LIBRARY}" != "0" ]; then
    echo "ERROR: The Energy Aware Runtime (EAR) was detected in the environment."
    echo "       This is known to conflict with GEOPM and must be disabled."
    RC=1
fi
if [ ! -z "${DARSHAN_PRELOAD}" ]; then
    echo "ERROR: Darshan was detected in your environment and is known to conflict with GEOPM."
    echo "Run \"module unload darshan\"."
    RC=1
fi

# Resource manager support
if command_available_and_executable srun; then
    # SLURM detected
    if ! 2>/dev/null srun --help | grep --quiet "cpu-bind"; then
        echo "ERROR: SLURM detected but is missing cpu-bind support."
        echo "       Please add TaskPlugin=task/affinity to the slurm.conf."
        RC=1
    fi
    TEST_GOVERNOR=performance
    ACTUAL_GOVERNOR="$(2>/dev/null srun --cpu-freq=${TEST_GOVERNOR} cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor)"
    if [ "${TEST_GOVERNOR}" != "${ACTUAL_GOVERNOR}" ]; then
        echo "WARNING: Unable to use SLURM and --cpu-freq to set the governor."
        echo "         Please ensure the current CPU governor settings do not impact"
        echo "         GEOPM's ability to control frequency."
        echo "         Tested: ${TEST_GOVERNOR} | Actual: ${ACTUAL_GOVERNOR}"
    fi
fi

if [ ${RC} -eq 0 ]; then
    echo "System is OK!"
fi
exit ${RC}
