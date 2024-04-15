#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys

found_metadata = False
try:
    import importlib_metadata as metadata
    from importlib_metadata import PackageNotFoundError
    found_metadata = True
except ImportError:
    try:
        from importlib import metadata
        from importlib.metadata import PackageNotFoundError
        found_metadata = True
    except ImportError:
        pass

def get_version():
    pkg_name = __name__.split('.')[0]
    if found_metadata:
        version = metadata.version(pkg_name)
    else:
        module_dir = os.path.dirname(os.path.realpath(__file__))
        version_path = os.path.join(module_dir, 'VERSION')
        with open(version_path) as fid:
            version = fid.read().strip()
        if version == "0.0.0":
            sys.stderr.print('Warning: Failed to determine version, setting version to "0.0.0"')
    return version

version = __version__ = get_version()
