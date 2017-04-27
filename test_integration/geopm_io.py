#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

import os
import json
import re
import pandas
import fnmatch

class AppOutput(object):
    def __init__(self, report_path, trace_base=None):
        self._reports = {}
        self._traces = {}
        self._all_paths = []
        self._all_paths.append(report_path)

        # Create a dict of <NODE_NAME> : <REPORT_OBJ>
        rr_size = os.stat(report_path).st_size
        rr = Report(report_path)
        self._reports[rr.get_node_name()] = rr
        while (rr.get_last_offset() != rr_size):
            rr = Report(report_path, rr.get_last_offset())
            if rr.get_node_name() is not None:
                self._reports[rr.get_node_name()] = rr

        if trace_base:
            trace_paths = [ff for ff in os.listdir('.')
                           if fnmatch.fnmatch(ff, trace_base + '*')]
            self._all_paths.extend(trace_paths)
            # Create a dict of <NODE_NAME> : <TRACE_DATAFRAME>
            self._traces = {tt.get_node_name(): tt.get_df() for tt in
                            [Trace(tp) for tp in trace_paths]}

    def remove_files(self):
        for ff in self._all_paths:
            try:
                os.remove(ff)
            except OSError:
                pass

    def get_node_names(self):
        return self._reports.keys()

    def get_report(self, node_name):
        return self._reports[node_name]

    def get_trace(self, node_name):
        return self._traces[node_name]


class Report(dict):
    def __init__(self, report_path, offset=0):
        super(Report, self).__init__()
        self._path = report_path
        self._offset = offset
        self._version = None
        self._name = None
        self._total_runtime = None
        self._total_energy = None
        self._total_ignore_runtime = None
        self._total_mpi_runtime = None
        self._node_name = None

        found_totals = False
        (region_name, region_id, runtime, energy, frequency, mpi_runtime, count) = None, None, None, None, None, None, None
        float_regex = r'([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)'

        with open(self._path, 'r') as fid:
            fid.seek(self._offset)
            line = fid.readline()
            while len(line) != 0:
                if self._version is None:
                    match = re.search(r'^##### geopm (\S+) #####$', line)
                    if match is not None:
                        self._version = match.group(1)
                if self._name is None:
                    match = re.search(r'^Profile: (\S+)$', line)
                    if match is not None:
                        self._name = match.group(1)
                if self._node_name is None:
                    match = re.search(r'^Host: (\S+)$', line)
                    if match is not None:
                        self._node_name = match.group(1)
                elif region_name is None:
                    match = re.search(r'^Region (\S+) \(([0-9]+)\):', line)
                    if match is not None:
                        region_name = match.group(1)
                        region_id = match.group(2)
                elif runtime is None:
                    match = re.search(r'^\s+runtime.+: ' + float_regex, line)
                    if match is not None:
                        runtime = float(match.group(1))
                elif energy is None:
                    match = re.search(r'^\s+energy.+: ' + float_regex, line)
                    if match is not None:
                        energy = float(match.group(1))
                elif frequency is None:
                    match = re.search(r'^\s+frequency.+: ' + float_regex, line)
                    if match is not None:
                        frequency = float(match.group(1))
                elif mpi_runtime is None:
                    match = re.search(r'^\s+mpi-runtime.+: ' + float_regex, line)
                    if match is not None:
                        mpi_runtime = float(match.group(1))
                elif count is None:
                    match = re.search(r'^\s+count: ' + float_regex, line)
                    if match is not None:
                        count = float(match.group(1))
                        self[region_name] = Region(region_name, region_id, runtime, energy, frequency, mpi_runtime, count)
                        (region_name, region_id, runtime, energy, frequency, mpi_runtime, count) = None, None, None, None, None, None, None
                if not found_totals:
                    match = re.search(r'^Application Totals:$', line)
                    if match is not None:
                        found_totals = True
                elif self._total_runtime is None:
                    match = re.search(r'\s+runtime.+: ' + float_regex, line)
                    if match is not None:
                        self._total_runtime = float(match.group(1))
                elif self._total_energy is None:
                    match = re.search(r'\s+energy.+: ' + float_regex, line)
                    if match is not None:
                        self._total_energy = float(match.group(1))
                elif self._total_mpi_runtime is None:
                    match = re.search(r'\s+mpi-runtime.+: ' + float_regex, line)
                    if match is not None:
                        self._total_mpi_runtime = float(match.group(1))
                elif self._total_ignore_runtime is None:
                    match = re.search(r'\s+ignore-time.+: ' + float_regex, line)
                    if match is not None:
                        self._total_ignore_runtime = float(match.group(1))
                        break # End of report blob

                line = fid.readline()
            self._offset = fid.tell()

        if (len(line) != 0 and (region_name is not None or not found_totals or
            None in (self._total_runtime, self._total_energy, self._total_ignore_runtime, self._total_mpi_runtime))):
            raise SyntaxError('Unable to parse report {} before offset {}: '.format(self._path, self._offset))

    def get_name(self):
        return self._name

    def get_version(self):
        return self._version

    def get_path(self):
        return self._path

    def get_runtime(self):
        return self._total_runtime

    def get_ignore_runtime(self):
        return self._total_ignore_runtime

    def get_mpi_runtime(self):
        return self._total_mpi_runtime

    def get_energy(self):
        return self._total_energy

    def get_node_name(self):
        return self._node_name

    def get_last_offset(self):
        return self._offset


class Region(object):
    def __init__(self, name, rid, runtime, energy, frequency, mpi_runtime, count):
        self._name = name
        self._id = rid
        self._runtime = runtime
        self._energy = energy
        self._frequency = frequency
        self._mpi_runtime = mpi_runtime
        self._count = count

    def __repr__(self):
        template = """\
{name} ({rid})
  runtime     : {runtime}
  energy      : {energy}
  frequency   : {frequency}
  mpi-runtime : {mpi_runtime}
  count       : {count}
"""
        return template.format(name=self._name,
                               rid=self._id,
                               runtime=self._runtime,
                               energy=self._energy,
                               frequency=self._frequency,
                               mpi_runtime=self._mpi_runtime,
                               count=self._count)

    def __str__(self):
        return self.__repr__()

    def get_name(self):
        return self._name

    def get_id(self):
        return self._id

    def get_runtime(self):
        return self._runtime

    def get_energy(self):
        return self._energy

    def get_frequency(self):
        return self._frequency

    def get_mpi_runtime(self):
        return self._mpi_runtime

    def get_count(self):
        return self._count


class Trace(object):
    def __init__(self, trace_path):
        self._path = trace_path
        self._df = pandas.read_table(trace_path, '|', dtype={'region_id ' : str})
        self._df.columns = list(map(str.strip, self._df[:0])) # Strip whitespace from column names
        self._df['region_id'] = self._df['region_id'].astype(str).map(str.strip) # Strip whitespace from region ID's
        self._node_name = trace_path.split('.trace-')[-1]

    def __repr__(self):
        return self._df.__repr__()

    def __str__(self):
        return self.__repr__()

    def __getattr__(self, attr):
        return getattr(self._df, attr)

    def __getitem__(self, key):
        return self._df.__getitem__(key)

    def get_df(self):
        return self._df

    def get_node_name(self):
        return self._node_name


class AppConf(object):
    def __init__(self, path):
        self._path = path
        self._loop_count = 1;
        self._region = []
        self._big_o = []
        self._hostname = []
        self._imbalance = []

    def set_loop_count(self, loop_count):
        self._loop_count = loop_count

    def append_region(self, name, big_o):
        self._region.append(name)
        self._big_o.append(big_o)

    def append_imbalance(self, hostname, imbalance):
        self._hostname.append(hostname)
        self._imbalance.append(imbalance)

    def get_path(self):
        return self._path

    def write(self):
        obj = {'loop-count' : self._loop_count,
               'region' : self._region,
               'big-o' : self._big_o}

        if (self._imbalance and self._hostname):
            obj['imbalance'] = self._imbalance
            obj['hostname'] = self._hostname

        with open(self._path, 'w') as fid:
            json.dump(obj, fid)


class CtlConf(object):
    def __init__(self, path, mode, options):
        self._path = path
        self._mode = mode
        self._options = options

    def set_tree_decider(self, decider):
        self._options['tree_decider'] = decider

    def set_leaf_decider(self, decider):
        self._options['leaf_decider'] = decider

    def set_platform(self, platform):
        self._options['platform'] = platform

    def set_power_budget(self, budget):
        self._options['power_budget'] = budget

    def get_path(self):
        return self._path

    def write(self):
        obj = {'mode' : self._mode,
               'options' : self._options}
        with open(self._path, 'w') as fid:
            json.dump(obj, fid)

