#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""The geopm daemon python package

"""

import os

try:
    from geopmdpy.version import __version__
except ImportError:
    try:
        # Look for VERSION file in git repository
        file_path = os.path.abspath(__file__)
        src_version_path = os.path.join(
                           os.path.dirname(
                           os.path.dirname(
                           os.path.dirname(file_path))),
                           'VERSION')
        with open(src_version_path) as fid:
            __version__ = fid.read().strip()
    except IOError:
        __version__ = '0.0.0'
