#!/bin/bash

RESOURCE="geopm-node-power-limit"
HOOK="geopm_power_limit"

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
