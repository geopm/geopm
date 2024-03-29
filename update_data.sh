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
