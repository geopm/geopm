#!/bin/bash
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

POWER_LIMIT_RESOURCE="geopm-node-power-limit"
MAX_POWER_LIMIT_RESOURCE="geopm-max-node-power-limit"
MIN_POWER_LIMIT_RESOURCE="geopm-min-node-power-limit"
JOB_POWER_LIMIT_RESOURCE="geopm-job-power-limit"
JOB_TYPE_RESOURCE="geopm-job-type"
MAX_SLOWDOWN_RESOURCE="geopm-max-slowdown"
SERVER_HOOK="geopm_power_limit_server"
REMOVE_OPT="--remove"

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
    server hook for queuejob events.

    Use the --remove option to uninstall the hook and related resources.
    "
}

install() {
    # Create all resources needed for the server side
    for resource in $POWER_LIMIT_RESOURCE $MAX_POWER_LIMIT_RESOURCE $MIN_POWER_LIMIT_RESOURCE $JOB_POWER_LIMIT_RESOURCE $MAX_SLOWDOWN_RESOURCE
    do
        out=`qmgr -c "list resource" | grep "$resource"`
        if [ -z "$out" ]; then
            echo "Creating $resource resource..."
            qmgr -c "create resource $resource type=float" || exit 1
        else
            echo "$resource resource already exists"
        fi
    done
    echo "Marking $JOB_POWER_LIMIT_RESOURCE as a server/queue resource..."
    qmgr -c "set resource $JOB_POWER_LIMIT_RESOURCE flag=q" || exit 1
    echo "Marking $MAX_POWER_LIMIT_RESOURCE and $MIN_POWER_LIMIT_RESOURCE as read-only resources..."
    qmgr -c "set resource $MAX_POWER_LIMIT_RESOURCE flag=r" || exit 1
    qmgr -c "set resource $MIN_POWER_LIMIT_RESOURCE flag=r" || exit 1

    out=`qmgr -c "list resource" | grep "$JOB_TYPE_RESOURCE"`
    if [ -z "$out" ]; then
        echo "Creating $JOB_TYPE_RESOURCE resource..."
        qmgr -c "create resource $JOB_TYPE_RESOURCE type=string" || exit 1
    else
        echo "$JOB_TYPE_RESOURCE resource already exists"
    fi

    if grep -q "^resources: .*$JOB_POWER_LIMIT_RESOURCE" "${PBS_HOME}/sched_priv/sched_config"; then
            echo "$JOB_POWER_LIMIT_RESOURCE is already defined as a consumable scheduler resource"
    else
            echo "Appending $JOB_POWER_LIMIT_RESOURCE as a consumable scheduler resource"
            sed -i -e "s/^resources: \"\([^\"]\+\)\"$/resources: \"\1, ${JOB_POWER_LIMIT_RESOURCE}\"/g" "${PBS_HOME}/sched_priv/sched_config"
    fi

    # Set up the server hook
    out=`qmgr -c "list hook" | grep "$SERVER_HOOK"`
    if [ -z "$out" ]; then
        echo "Creating $SERVER_HOOK hook..."
        qmgr -c "create hook $SERVER_HOOK" || exit 1
    else
        echo "$SERVER_HOOK hook already exists"
    fi
    echo "Importing and configuring server hook..."
    qmgr -c "import hook $SERVER_HOOK application/x-python default $SERVER_HOOK.py" || exit 1
    qmgr -c "set hook $SERVER_HOOK event='queuejob,modifyjob'" || exit 1

    echo "Done."
}

remove() {
    if grep -q "^resources: .*$JOB_POWER_LIMIT_RESOURCE" "${PBS_HOME}/sched_priv/sched_config"; then
            echo "Removing $JOB_POWER_LIMIT_RESOURCE from the list of consumable scheduler resources"
            sed -i -e "s/^resources: \"\([^\"]\+\), ${JOB_POWER_LIMIT_RESOURCE}\(, [^\"]\+\)\?\"$/resources: \"\1\2\"/g" "${PBS_HOME}/sched_priv/sched_config"
    else
            echo "$JOB_POWER_LIMIT_RESOURCE not found in the list of consumable scheduler resources"
    fi

    # Remove resources
    for resource in $POWER_LIMIT_RESOURCE $MAX_POWER_LIMIT_RESOURCE $MIN_POWER_LIMIT_RESOURCE $JOB_POWER_LIMIT_RESOURCE $MAX_SLOWDOWN_RESOURCE $JOB_TYPE_RESOURCE
    do
        out=`qmgr -c "list resource" | grep "$resource"`
        if [ -z "$out" ]; then
            echo "$resource resource not found"
        else
            echo "Removing $resource resource..."
            qmgr -c "delete resource $resource" || exit 1
        fi
    done

    # Remove the server hook
    out=`qmgr -c "list hook" | grep "$SERVER_HOOK"`
    if [ -z "$out" ]; then
        echo "$SERVER_HOOK hook not found"
    else
        echo "Removing $SERVER_HOOK hook..."
        qmgr -c "delete hook $SERVER_HOOK" || exit 1
    fi

    echo "Done."
}

if [ $# -eq 0 ]; then
    install
    echo "$SCHED_CONFIG_MODIFIED_MESSAGE"
    echo ""
    echo "GEOPM PBS server hook has been installed but is not yet configured."
    echo "To set a power limit across jobs, set resources_available for $JOB_POWER_LIMIT_RESOURCE"
    echo "To set a minimum node power limit, use $MIN_POWER_LIMIT_RESOURCE"
    echo "To set a maximum node power limit, use $MAX_POWER_LIMIT_RESOURCE"
    echo "To set a maximum job slowdown tolerance, use $MAX_SLOWDOWN_RESOURCE"
    echo "Example to set a power limit: qmgr -c 'set server resources_available.${JOB_POWER_LIMIT_RESOURCE}=<max sum of node power (W)>'"
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
