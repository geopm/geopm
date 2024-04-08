#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import importlib_metadata as metadata

def get_version():
    pkg_name = __name__.split('.')[0]
    version = metadata.version(pkg_name)
    return version

version = __version__ = get_version()
