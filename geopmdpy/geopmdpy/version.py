#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys

def get_version():
    module_dir = os.path.dirname(os.path.realpath(__file__))
    version_path = os.path.join(module_dir, 'VERSION')
    try:
        with open(version_path) as fid:
            version = fid.read().strip()
    except FileNotFoundError:
        sys.stderr.write(f'Warning: importing {__file__} from an uninstalled module, setting version to "0.0.0"\n\n')
        version = '0.0.0'
    return version

version = __version__ = get_version()
