#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from geopmdpy.gffi import gffi
from geopmdpy.gffi import get_dl_geopm

gffi.cdef("""
uint64_t geopm_crc32_str(const char *key);
""")
try:
    _dl = get_dl_geopm()
except OSError as ee:
    raise OSError('This module requires libgeopm.so to be present in your LD_LIBRARY_PATH.') from ee

def crc32_str(key):
    """Return the geopm hash of a string

    Args:
        key (int): String to hash

    Returns:
        int: Hash of string

    """
    global gffi
    global _dl

    key_name_cstr = gffi.new("char[]", key.encode())
    return _dl.geopm_crc32_str(key_name_cstr)
