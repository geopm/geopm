#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

def __getattr__(name):
    if name == "__version__":
        from importlib import metadata
        return metadata.version("geopmdpy")

