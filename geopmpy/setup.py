#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import setuptools

package_name = 'geopmpy'

try:
    from setuptools_scm import get_version
    script_dir = os.path.dirname(os.path.realpath(__file__))
    version = get_version(f'{script_dir}/..')
    with open(f'{script_dir}/{package_name}/VERSION', 'w') as fid:
        fid.write(version)
except (ImportError, LookupError):
    pass

setuptools.setup()

