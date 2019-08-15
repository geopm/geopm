#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

from __future__ import absolute_import

from builtins import range
import pandas

def make_mock_report_df(name_prefix, node_names, agent_params):
    """ Create a mock report dataframe.
    Arguments:
    name_prefix (str)
        Prefix for the profile name
    node_names (list(str))
        List of nodes to include in the report
    agent_params (dict(dict(lambda)))
        Dictionary with agents as keys and dictionaries as values.  The nested
        dictionaries map reported value names to lambda functions
        (node, region, agent_param)->value.
    """
    version = '0.3.0'
    region_id = {
        'epoch':  '9223372036854775808',
        'dgemm':  '11396693813',
        'stream': '20779751936'
    }
    start_time = 'Tue Nov  6 08:00:00 2018'
    index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration', 'region']
    iterations = list(range(1, 4))

    input_data = {}
    for agent in agent_params:
        gen_val, params = agent_params[agent]
        for col in gen_val:
            if col not in input_data:
                input_data[col] = {}
            for param in params:
                prof_name = name_prefix if params == [None] else '{}_{}'.format(name_prefix, param)
                for node_name in node_names:
                    for it in iterations:
                        for region in region_id:
                            gen_val['id'] = lambda node, region, param: region_id[region]
                            index = (version, start_time, prof_name, agent, node_name, it, region)
                            value = gen_val[col](node_name, region, param)
                            input_data[col][index] = value

    df = pandas.DataFrame.from_dict(input_data)
    df.index.rename(index_names, inplace=True)
    return df
