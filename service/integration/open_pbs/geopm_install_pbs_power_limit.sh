#!/bin/bash
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

RESOURCE="geopm-node-power-limit"
HOOK="geopm_power_limit"
REMOVE_OPT="--remove"
SAVED_CONTROLS_BASE_DIR="/run/geopm-pbs-hooks"

print_usage() {
    echo "
    Usage: $0 [${REMOVE_OPT}]

    Invoking this script with no arguments installs the GEOPM power limit
    hook.

    Use the --remove option to uninstall the hook. This will also remove the
    $RESOURCE resource and the saved controls directory.
    "
}

install() {
    out=`qmgr -c "list resource" | grep "$RESOURCE"`
    if [ -z "$out" ]; then
        echo "Creating $RESOURCE resource..."
        qmgr -c "create resource $RESOURCE type=float" || exit 1
    else
        echo "$RESOURCE resource already exists"
    fi

    out=`qmgr -c "list hook" | grep "$HOOK"`
    if [ -z "$out" ]; then
        echo "Creating $HOOK hook..."
        qmgr -c "create hook $HOOK" || exit 1
    else
        echo "$HOOK hook already exists"
    fi
    echo "Importing and configuring hook..."
    qmgr -c "import hook $HOOK application/x-python default ${HOOK}.py" || exit 1
    qmgr -c "set hook $HOOK event='execjob_prologue,execjob_epilogue'" || exit 1

    echo "Done."
}

remove() {
    out=`qmgr -c "list resource" | grep "$RESOURCE"`
    if [ -z "$out" ]; then
        echo "$RESOURCE resource not found"
    else
        echo "Removing $RESOURCE resource..."
        qmgr -c "delete resource $RESOURCE" || exit 1
    fi

    out=`qmgr -c "list hook" | grep "$HOOK"`
    if [ -z "$out" ]; then
        echo "$HOOK hook not found"
    else
        echo "Removing $HOOK hook..."
        qmgr -c "delete hook $HOOK" || exit 1
    fi

    rm -rf "$SAVED_CONTROLS_BASE_DIR"

    echo "Done."
}

if [ $# -eq 0 ]; then
    install
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
