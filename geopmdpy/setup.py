#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import setuptools
import os

package_name = 'geopmdpy'

try:
    from setuptools_scm import get_version
    script_dir = os.path.dirname(os.path.realpath(__file__))
    version = get_version(f'{script_dir}/..')
    with open(f'{script_dir}/{package_name}/VERSION', 'w') as fid:
        fid.write(version)
except (ImportError, LookupError):
    with open(f'{script_dir}/{package_name}/VERSION', 'r') as fid:
        version = fid.read().strip()
    os.environ['SETUPTOOLS_SCM_PRETEND_VERSION'] = version

setuptools.setup()

