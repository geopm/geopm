#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys

def get_version():
    module_dir = os.path.dirname(os.path.realpath(__file__))
    version_path = os.path.join(module_dir, 'VERSION')
    with open(version_path) as fid:
        version = fid.read().strip()
    return version

version = __version__ = get_version()
