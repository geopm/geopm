#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Helper script that exports data embedded in python source files.

set -e -x
# Recreate the JSON schema files in case they have changed,
# remember to git commit the generated files if they have.
PYTHONPATH=geopmdpy python3 -m geopmdpy.schemas docs/json_schemas
# Update io.github.geopm.xml in case API documentation changed,
# remember to git commit the generated files if they have.
PYTHONPATH=geopmdpy python3 -m geopmdpy.dbus_xml > libgeopmd/io.github.geopm.xml

# Recreate the C++ files with json data in case they have changed,
# remember to git commit the generated files if they have.
HEADER0="// THIS IS A GENERATED FILE, DO NOT MODIFY, INSTEAD MODIFY: "
HEADER1="// AND RERUN update_data.sh"
for DATA in docs/json_data/msr_data_*.json; do
    JSON_IDENTIFIER=$(echo ${DATA} | sed -e 's|docs/json_data/msr_data_\([a-z]*\)\.json|\1|')
    INPUT=libgeopmd/src/json_data.cpp.in
    OUTPUT=libgeopmd/src/msr_data_${JSON_IDENTIFIER}.cpp
    echo ${HEADER0} ${DATA} > ${OUTPUT}
    echo ${HEADER1} >> ${OUTPUT}
    sed -e '/@JSON_CONTENTS@/ {' \
	-e "r $DATA" \
	-e 'd' -e '}' \
	-e "s/@JSON_IDENTIFIER@/$JSON_IDENTIFIER\_msr_json/g" \
	${INPUT} >> ${OUTPUT}
done

for DATA in docs/json_data/sysfs_attributes_*.json; do
    JSON_IDENTIFIER=$(echo ${DATA} | sed -e 's|docs/json_data/sysfs_attributes_\([a-z]*\)\.json|\1|')
    INPUT=libgeopmd/src/json_data.cpp.in
    OUTPUT=libgeopmd/src/sysfs_attributes_${JSON_IDENTIFIER}.cpp
    echo ${HEADER0} ${DATA} > ${OUTPUT}
    echo ${HEADER1} >> ${OUTPUT}
    sed -e '/@JSON_CONTENTS@/ {' \
	-e "r $DATA" \
	-e 'd' -e '}' \
	-e "s/@JSON_IDENTIFIER@/$JSON_IDENTIFIER\_sysfs_json/g" \
	${INPUT} > ${OUTPUT}
done
