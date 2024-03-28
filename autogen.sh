#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

set -e -x
# Create python source distribution (provides geopmdpy/version.py)
python3 -m build --sdist geopmdpy
python3 -m build --sdist geopmpy

# Recreate the JSON schema files in case they have changed,
# remember to git commit the generated files if they have.
PYTHONPATH=geopmdpy python3 -m geopmdpy.schemas docs/json_schemas
# Update io.github.geopm.xml in case API documentation changed,
# remember to git commit the generated files if they have.
PYTHONPATH=geopmdpy python3 -m geopmdpy.dbus_xml > libgeopmd/io.github.geopm.xml

# Create VERSION file
if [ ! -e VERSION ]; then
    PYTHONPATH=geopmdpy python3 -c "from geopmdpy.version import __version__; print(__version__)" > VERSION
    # TODO: If required our old VERSION format can be created this way
    # PYTHONPATH=geopmdpy python3 -c "from geopmdpy.version import __version__; vv=__version__.replace('.dev','+dev', 1).replace('+g','g', 1); vv=vv[:vv.rfind('.d20')]; print(vv)" > VERSION
    if [ $? -ne 0 ]; then
        echo "WARNING:  VERSION file does not exist and geopmdpy/version.py failed, setting version to 0.0.0" 1>&2
        echo "0.0.0" > VERSION
    fi
fi

set +x
# Copy repository files from base directory
for ff in AUTHORS ChangeLog CODE_OF_CONDUCT.md CONTRIBUTING.rst COPYING COPYING-TPP SECURITY.md VERSION; do
    for dd in libgeopmd libgeopm geopmdpy geopmpy; do
        if [ ! -f $dd/$ff ]; then
            cp $ff $dd/$ff
	fi
    done
done

set -x
# Create configure scripts
cd libgeopmd && autoreconf -i -f; cd -
cd libgeopm && autoreconf -i -f; cd -
