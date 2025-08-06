#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Resources needed for the compute nodes
NODE_RESOURCE="geopm-node-power-limit"
JOB_RESOURCE="geopm-job-power-limit"
NODE_CAP_HOOK="geopm_power_limit"
REMOVE_OPT="--remove"
SAVED_CONTROLS_BASE_DIR="/run/geopm/pbs-hooks"

# Set the location of PBS_HOME by sourcing the PBS config.
source '/etc/pbs.conf'

print_usage() {
    echo "
    Usage: $0 [${REMOVE_OPT}]

    Invoking this script with no arguments installs the GEOPM power limit
    compute hook for prologue and epilogue events.

    Use the --remove option to uninstall the hook and remove the saved controls directory.
    "
}

install() {
    # Create directories for saved controls
    mkdir -p "${SAVED_CONTROLS_BASE_DIR}/SAVE_FILES"
    chmod 755 "${SAVED_CONTROLS_BASE_DIR}"
    chmod 755 "${SAVED_CONTROLS_BASE_DIR}/SAVE_FILES"

    # Set up the prologue/epilogue hook
    out=`qmgr -c "list hook" | grep "$NODE_CAP_HOOK"`
    if [ -z "$out" ]; then
        echo "Creating $NODE_CAP_HOOK hook..."
        qmgr -c "create hook $NODE_CAP_HOOK" || exit 1
    else
        echo "$NODE_CAP_HOOK hook already exists"
    fi
    echo "Importing and configuring prologue/epilogue hook..."
    qmgr -c "import hook $NODE_CAP_HOOK application/x-python default geopm_power_limit_compute.py" || exit 1
    qmgr -c "set hook $NODE_CAP_HOOK event='execjob_prologue,execjob_epilogue'" || exit 1

    echo "Done."
}

remove() {
    # Remove the prologue/epilogue hook
    out=`qmgr -c "list hook" | grep "$NODE_CAP_HOOK"`
    if [ -z "$out" ]; then
        echo "$NODE_CAP_HOOK hook not found"
    else
        echo "Removing $NODE_CAP_HOOK hook..."
        qmgr -c "delete hook $NODE_CAP_HOOK" || exit 1
    fi

    # Remove saved controls directory
    rm -rf "$SAVED_CONTROLS_BASE_DIR"

    echo "Done."
}

if [ $# -eq 0 ]; then
    install
    echo ""
    echo "GEOPM PBS compute node hook has been installed."
    echo "Note: This hook uses resources that must be defined on the PBS server."
    echo "Make sure to run the server installation script on the PBS server."
elif [ $# -eq 1 ]; then
    if [ "$1" == "$REMOVE_OPT" ]; then
        remove
    else
        echo "Unrecognized option: $1"
        print_usage
    fi
else
    echo "Invalid number of arguments"
    print_usage
fi
