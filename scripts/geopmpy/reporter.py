#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from geopmdpy.gffi import gffi
from geopmdpy.gffi import get_dl_geopm
from geopmdpy import error


gffi.cdef("""
int geopm_reporter_init(void);
int geopm_reporter_update(void);
int geopm_reporter_generate(const char *profile_name,
                            const char *agent_name,
                            size_t result_max,
                            char *result);
""")
_dl = get_dl_geopm()

def init():
    '''Initialize reporter object

    This call pushes all required signals onto the batch requests for
    ``geopmdpy.pio``.  This also sets the start time for the report.

    '''
    global _dl
    err = _dl.geopm_reporter_init()
    if err != 0:
        raise RuntimeError('geopm_reporter_init() failed: {}'.format(error.message(err)))

def update():
    '''Update aggregated statistics

    The update to the aggregated statistics is based on the values
    last read by ``geopmdpy.pio.read_batch()``.

    '''
    global _dl
    err = _dl.geopm_reporter_update()
    if err != 0:
        raise RuntimeError('geopm_reporter_init() failed: {}'.format(error.message(err)))

def generate(profile_name, agent_name):
    '''Create a YAML report based on all aggregated data

    Returns a string representing a YAML report reflecting the data
    gathered over time with calls to the ``update()`` method.

    Args:
        profile_name (str): String that will be added to the report
                     in the ``Profile`` field in the report header.

        agent_name (str): String that will be added to the report in
                   the ``Agent`` field in the report header.

    Returns:
        str: YAML report containing header with meta-data and the aggregated measurements made during execution.

    '''
    global gffi
    global _dl
    profile_name_cstr = gffi.new("char[]", profile_name.encode())
    agent_name_cstr = gffi.new("char[]", agent_name.encode())
    result_max = 2 * 1024 * 1024
    result_cstr = gffi.new("char[]", result_max)
    err = _dl.geopm_reporter_generate(profile_name_cstr, agent_name_cstr,
                                      result_max, result_cstr)
    if err != 0:
        raise RuntimeError('geopm_reporter_generate() failed: {}'.format(error.message(err)))
    return gffi.string(result_cstr).decode()
