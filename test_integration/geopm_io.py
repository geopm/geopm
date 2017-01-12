#!/usr/bin/env python
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

import json
import re


class Report(dict):
    def __init__(self, report_path):
        super(Report, self).__init__()
        self._path = report_path
        self._version = None
        self._name = None
        self._total_runtime = None
        self._total_energy = None
        self._total_mpi_runtime = None
        found_totals = False
        (region_name, runtime, energy, frequency, count) = None, None, None, None, None
        float_regex = r'([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)'

        with open(self._path, 'r') as fid:
            for line in fid:
                if self._version is None:
                    match = re.search(r'^##### geopm (\S+) #####$', line)
                    if match is not None:
                        self._version = match.group(1)
                elif self._name is None:
                    match = re.search(r'^Profile: (\S+)$', line)
                    if match is not None:
                        self._name = match.group(1)
                elif region_name is None:
                    match = re.search(r'^Region (\S+) \([0-9]+\):', line)
                    if match is not None:
                        region_name = match.group(1)
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
                elif count is None:
                    match = re.search(r'^\s+count: ' + float_regex, line)
                    if match is not None:
                        count = float(match.group(1))
                        self[region_name] = Region(region_name, runtime, energy, frequency, count)
                        (region_name, runtime, energy, frequency, count) = None, None, None, None, None
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

        if (region_name is not None or not found_totals or
            None in (self._name, self._version, self._total_runtime, self._total_energy, self._total_mpi_runtime)):
            raise SyntaxError('Unable to parse file: ' + self._path)

    def get_name(self):
        return self._name

    def get_version(self):
        return self._version

    def get_path(self):
        return self._path

    def get_runtime(self):
        return self._total_runtime

    def get_mpi_runtime(self):
        return self._total_mpi_runtime

    def get_energy(self):
        return self._total_energy


class Region(object):
    def __init__(self, name, runtime, energy, frequency, count):
        self._name = name
        self._runtime = runtime
        self._energy = energy
        self._frequency = frequency
        self._count = count

    def __repr__(self):
        template = """\
{name}
  Runtime   : {runtime}
  Energy    : {energy}
  Frequency : {frequency}
  Count     : {count}
"""
        return template.format(name=self._name,
                               runtime=self._runtime,
                               energy=self._energy,
                               frequency=self._frequency,
                               count=self._count)

    def __str__(self):
        return self.__repr__()

    def get_name(self):
        return self._name

    def get_runtime(self):
        return self._runtime

    def get_energy(self):
        return self._energy

    def get_frequency(self):
        return self._frequency

    def get_count(self):
        return self._count

class Trace(object):
    def __init__(self, trace_path):
        raise NotImplementedError

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

