#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import importlib_metadata as metadata
from importlib_metadata import PackageNotFoundError

def get_version():
    pkg_name = __name__.split('.')[0]
    try:
        version = metadata.version(pkg_name)
    except PackageNotFoundError as ex:
        sys.stderr.write(f'Warning: {ex}, setting version to "0.0.0"')
        version = '0.0.0'
    return version

version = __version__ = get_version()
