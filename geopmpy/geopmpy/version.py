#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys

def get_version():
    module_dir = os.path.dirname(os.path.realpath(__file__))
    try:
        pkg_name = __name__.split('.')[0]
        # If in git repo, always use for verioning
        from setuptools_scm import get_version
        version = get_version(f'{module_dir}/../..')
    except (ImportError, LookupError):
        # Fall back to VERSION file
        version_path = os.path.join(module_dir, 'VERSION')
        try:
            with open(version_path) as fid:
                version = fid.read().strip()
        except FileNotFoundError:
            # Fallback to metadata if python version is new enough
            try:
                from importlib import metadata
                version = metadata.version(pkg_name)
            except ImportError:
                # If all else fails print a warning and return 0.0.0
                sys.stderr.write('Warning: Unable to determine GEOPM version, setting to 0.0.0\n\n')
                version = '0.0.0'
    return version

version = __version__ = get_version()
