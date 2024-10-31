#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from . import gffi

try:
    _dl = gffi.get_dl_geopmd()
except OSError as ee:
    raise OSError('This module requires libgeopm.so to be present in your LD_LIBRARY_PATH.') from ee

def hash_str(key):
    """Return the geopm hash of a string

    Args:
        key (int): String to hash

    Returns:
        int: Hash of string

    """
    key_name_cstr = gffi.gffi.new("char[]", key.encode())
    return _dl.geopm_crc32_str(key_name_cstr)
