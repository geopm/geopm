#
#  Copyright (c) 2015 - 2022, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from geopmdpy.gffi import gffi
from geopmdpy.gffi import get_dl_geopmpolicy
from geopmdpy import error


gffi.cdef("""
int geopm_reporter_init(void);
int geopm_reporter_update(void);
int geopm_reporter_generate(const char *profile_name,
                            const char *agent_name,
                            size_t result_max,
                            char *result);
""")
_dl = get_dl_geopmpolicy()

def init():
    '''Initialize reporter object

    This call pushes all required signals onto the batch requests for
    geopmdpy.pio.  This also sets the start time for the report.

    '''
    global _dl
    err = _dl.geopm_reporter_init()
    if err != 0:
        raise RuntimeError('geopm_reporter_init() failed: {}'.format(error.message(err)))

def update():
    '''Update aggregated statistics

    The update to the aggregated statistics is based on the values
    last read by geopmdpy.pio.read_batch().

    '''
    global _dl
    err = _dl.geopm_reporter_update()
    if err != 0:
        raise RuntimeError('geopm_reporter_init() failed: {}'.format(error.message(err)))

def generate(profile_name, agent_name):
    '''Create a YAML report based on all aggregated data

    Returns a string representing a YAML report reflecting the data
    gathered over time with calls to the update() method.

    Args:
        profile_name (str): String that will be added to the report
                     in the Profile field in the report header.

        agent_name (str): String that will be added to the report in
                   the Agent field in the report header.

    Returns:
        str: YAML report containing header with meta-data and the
             aggregated measurements made during execution.

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
