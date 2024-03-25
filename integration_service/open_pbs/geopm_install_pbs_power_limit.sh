#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

NODE_RESOURCE="geopm-node-power-limit"
NODE_MAX_RESOURCE="geopm-max-node-power-limit"
NODE_MIN_RESOURCE="geopm-min-node-power-limit"
JOB_RESOURCE="geopm-job-power-limit"
JOB_TYPE_RESOURCE="geopm-job-type"
DEFAULT_SLOWDOWN_RESOURCE="geopm-default-slowdown"
NODE_CAP_HOOK="geopm_power_limit"
REMOVE_OPT="--remove"
SAVED_CONTROLS_BASE_DIR="/run/geopm/pbs-hooks"

SCHED_CONFIG_MODIFIED_MESSAGE="\
Note: ${PBS_HOME}/sched_priv/sched_config has been modified, but modifications
have not been applied to running PBS instance. Restart the PBS scheduler
process for changes to take effect."

# Set the location of PBS_HOME by sourcing the PBS config.
source '/etc/pbs.conf'

print_usage() {
    echo "
    Usage: $0 [${REMOVE_OPT}]

    Invoking this script with no arguments installs the GEOPM power limit
    hook.

    Use the --remove option to uninstall the hook. This will also remove the
    $NODE_RESOURCE resource and the saved controls directory.
    "
}

install() {
    for resource in $NODE_RESOURCE $NODE_MAX_RESOURCE $NODE_MIN_RESOURCE $JOB_RESOURCE $DEFAULT_SLOWDOWN_RESOURCE
    do
        out=`qmgr -c "list resource" | grep "$resource"`
        if [ -z "$out" ]; then
            echo "Creating $resource resource..."
            qmgr -c "create resource $resource type=float" || exit 1
        else
            echo "$resource resource already exists"
        fi
    done
    echo "Marking $JOB_RESOURCE as a server/queue resource..."
    qmgr -c "set resource $JOB_RESOURCE flag=q" || exit 1
    echo "Marking $NODE_MAX_RESOURCE and $NODE_MIN_RESOURCE as read-only resources..."
    qmgr -c "set resource $NODE_MAX_RESOURCE flag=r" || exit 1
    qmgr -c "set resource $NODE_MIN_RESOURCE flag=r" || exit 1

    out=`qmgr -c "list resource" | grep "$JOB_TYPE_RESOURCE"`
    if [ -z "$out" ]; then
        echo "Creating $JOB_TYPE_RESOURCE resource..."
        qmgr -c "create resource $JOB_TYPE_RESOURCE type=string" || exit 1
    else
        echo "$JOB_TYPE_RESOURCE resource already exists"
    fi

    if grep -q "^resources: .*$JOB_RESOURCE" "${PBS_HOME}/sched_priv/sched_config"; then
            echo "$JOB_RESOURCE is already defined as a consumable scheduler resource"
    else
            echo "Appending $JOB_RESOURCE as a consumable scheduler resource"
            sed -i -e "s/^resources: \"\([^\"]\+\)\"$/resources: \"\1, ${JOB_RESOURCE}\"/g" "${PBS_HOME}/sched_priv/sched_config"
    fi

    out=`qmgr -c "list hook" | grep "$NODE_CAP_HOOK"`
    if [ -z "$out" ]; then
        echo "Creating $NODE_CAP_HOOK hook..."
        qmgr -c "create hook $NODE_CAP_HOOK" || exit 1
    else
        echo "$NODE_CAP_HOOK hook already exists"
    fi
    echo "Importing and configuring hook..."
    qmgr -c "import hook $NODE_CAP_HOOK application/x-python default ${NODE_CAP_HOOK}.py" || exit 1
    qmgr -c "set hook $NODE_CAP_HOOK event='queuejob,runjob,execjob_prologue,execjob_epilogue'" || exit 1

    echo "Done."
}

remove() {
    if grep -q "^resources: .*$JOB_RESOURCE" "${PBS_HOME}/sched_priv/sched_config"; then
            echo "Removing $JOB_RESOURCE from the list of consumable scheduler resources"
            sed -i -e "s/^resources: \"\([^\"]\+\), ${JOB_RESOURCE}\(, [^\"]\+\)\?\"$/resources: \"\1\2\"/g" "${PBS_HOME}/sched_priv/sched_config"
    else
            echo "$JOB_RESOURCE not found in the list of consumable scheduler resources"
    fi

    for resource in $NODE_RESOURCE $NODE_MAX_RESOURCE $NODE_MIN_RESOURCE $JOB_RESOURCE $DEFAULT_SLOWDOWN_RESOURCE $JOB_TYPE_RESOURCE
    do
        out=`qmgr -c "list resource" | grep "$resource"`
        if [ -z "$out" ]; then
            echo "$resource resource not found"
        else
            echo "Removing $resource resource..."
            qmgr -c "delete resource $resource" || exit 1
        fi
    done

    out=`qmgr -c "list hook" | grep "$NODE_CAP_HOOK"`
    if [ -z "$out" ]; then
        echo "$NODE_CAP_HOOK hook not found"
    else
        echo "Removing $NODE_CAP_HOOK hook..."
        qmgr -c "delete hook $EXECHOST_HOOK" || exit 1
    fi

    rm -rf "$SAVED_CONTROLS_BASE_DIR"

    echo "Done."
}

if [ $# -eq 0 ]; then
    install
    echo "$SCHED_CONFIG_MODIFIED_MESSAGE"
    echo ""
    echo "GEOPM PBS plugins have been installed but are not yet configured."
    echo "To set a power limit across jobs, set resources_available for $JOB_RESOURCE"
    echo "To set a minimum node power limit, use $NODE_MIN_RESOURCE"
    echo "To set a maximum node power limit, use $NODE_MAX_RESOURCE"
    echo "To set a default job slowdown target, use $DEFAULT_SLOWDOWN_RESOURCE"
    echo "Example to set a power limit: qmgr -c 'set server resources_available.${JOB_RESOURCE}=<max sum of node power (W)>'"
elif [ $# -eq 1 ]; then
    if [ "$1" == "$REMOVE_OPT" ]; then
        remove
        echo "$SCHED_CONFIG_MODIFIED_MESSAGE"
    else
        echo "Unrecognized option: $1"
        print_usage
    fi
else
    echo "Invalid number of arguments"
    print_usage
fi
