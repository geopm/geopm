#!/bin/bash
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

# Copy repository files from base directory
for ff in AUTHORS CODE_OF_CONDUCT.md CONTRIBUTING.rst COPYING COPYING-TPP; do
    if [ ! -f $ff ]; then
        cp ../$ff .
    fi
done
python3 -m build --sdist
if [ ! -e VERSION ]; then
    PYTHONPATH=: \
    python3 -c "from geopmdpy.version import __version__; vv=__version__.replace('.dev','+dev', 1).replace('+g','g', 1); vv=vv[:vv.rfind('.d20')]; print(vv)" > VERSION || \
    echo "0.0.0" > VERSION && \
    echo "WARNING:  VERSION file does not exist and git describe failed, setting version to 0.0.0" 1>&2
fi
# Recreate the JSON schema files in case they have changed, remember to git commit the generated files if they have.
python3 geopmdpy/schemas.py docs/json_schemas
# Create configure script
autoreconf -i -f
