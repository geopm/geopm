#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys

def get_version():
    # Try VERSION file
    module_dir = os.path.dirname(os.path.realpath(__file__))
    version_path = os.path.join(module_dir, 'VERSION')
    with open(version_path) as fid:
        version = fid.read().strip()
    if version == '0.0.0':
        pkg_name = __name__.split('.')[0]
        # If in git repo, always use for verioning
        try:
            from setuptools_scm import get_version
            version = get_version(f'{module_dir}/../..')
        except ImportError:
            # If all else fails print a warning and return 0.0.0
            sys.stderr.write('Warning: Unable to determine GEOPM version, setting to 0.0.0\n\n')
    return version

version = __version__ = get_version()
