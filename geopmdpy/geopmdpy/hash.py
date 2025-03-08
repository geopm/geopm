#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from . import gffi

def hash_str(key):
    """Return the geopm hash of a string

    Args:
        key (int): String to hash

    Returns:
        int: Hash of string

    """
    key_name_cstr = gffi.gffi.new("char[]", key.encode())
    return gffi.dl_geopmd.geopm_crc32_str(key_name_cstr)
