#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

COMPUTE_HOOK="geopm_power_limit_compute"
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
    # Set up the prologue/epilogue hook
    out=`qmgr -c "list hook" | grep "$COMPUTE_HOOK"`
    if [ -z "$out" ]; then
        echo "Creating $COMPUTE_HOOK hook..."
        qmgr -c "create hook $COMPUTE_HOOK" || exit 1
    else
        echo "$COMPUTE_HOOK hook already exists"
    fi
    echo "Importing and configuring prologue/epilogue hook..."
    qmgr -c "import hook $COMPUTE_HOOK application/x-python default $COMPUTE_HOOK.py" || exit 1
    qmgr -c "set hook $COMPUTE_HOOK event='execjob_prologue,execjob_epilogue'" || exit 1

    echo "Done."
}

remove() {
    # Remove the prologue/epilogue hook
    out=`qmgr -c "list hook" | grep "$COMPUTE_HOOK"`
    if [ -z "$out" ]; then
        echo "$COMPUTE_HOOK hook not found"
    else
        echo "Removing $COMPUTE_HOOK hook..."
        qmgr -c "delete hook $COMPUTE_HOOK" || exit 1
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
