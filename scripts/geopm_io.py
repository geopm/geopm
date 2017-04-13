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
    def __init__(self, report_base, trace_base=None):
        self._reports = {}
        self._reports_df = pandas.DataFrame()
        self._traces = {}
        self._all_paths = []
        self._reports_df_list = []
        self._index_tracker = IndexTracker()

        dir_name = os.path.dirname(report_base) if os.path.dirname(report_base) else '.'
        report_pattern = os.path.basename(report_base)
        if not report_pattern:
            report_pattern = '*report*'
        else:
            report_pattern += '*'
        report_files = [ff for ff in os.listdir(dir_name)
                        if fnmatch.fnmatch(ff, report_pattern)]

        if len(report_files) == 0:
            raise RuntimeError('No report files found with pattern {} in directory {}.'.format(report_pattern, dir_name))

        # Create a dict of <NODE_NAME> : <REPORT_OBJ>; Create DF
        for rf in report_files:
            rp = os.path.join(dir_name, rf)
            self._all_paths.append(rp)

            # Parse the first report
            rr_size = os.stat(rp).st_size
            rr = Report(rp)
            self.add_report_df(rr)
            self._reports[rr.get_node_name()] = rr

            # Parse the remaining reports in this file
            while (rr.get_last_offset() != rr_size):
                rr = Report(rp, rr.get_last_offset())
                if rr.get_node_name() is not None:
                    self.add_report_df(rr)
                    self._reports[rr.get_node_name()] = rr
            Report.reset_vars() # If we've reached the end of a report, reset the static vars

        self._reports_df = pandas.concat(self._reports_df_list)
        self._reports_df = self._reports_df.sort_index(ascending=True)

        # TODO : How am I going to determine the index for the trace files if none of the decider information
        #        is available?

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

    def add_report_df(self, rr):
        # Build and index the DF
        rdf = pandas.DataFrame(rr).T.drop('name', 1)
        numeric_cols = ['count', 'energy', 'frequency', 'mpi_runtime', 'runtime']
        rdf[numeric_cols] = rdf[numeric_cols].apply(pandas.to_numeric)

        # Add extra index info
        rdf = rdf.set_index(self._index_tracker.get_multiindex(rr))
        self._reports_df_list.append(rdf)

    def get_node_names(self):
        return self._reports.keys()

    def get_report(self, node_name):
        return self._reports[node_name]

    def get_trace(self, node_name):
        return self._traces[node_name]

    def get_report_df(self):
        return self._reports_df


class IndexTracker(object):
    """
    Maintains a dict of what unique indicies have been parsed already.  This is used to keep a count of
    how many times a particular experiment has been performed.  The current unique tuple to identify an
    experiment is:
    (<GEOPM_VERSION>, <PROFILE_NAME>, <POWER_BUDGET> , <TREE_DECIDER>, <LEAF_DECIDER>, <NODE_NAME>)
    """
    def __init__ (self):
        self._reports = {}

    def _check_increment(self, report):
        """
        Checks to see if the current report has been seen before.  If so, the count is incremented.  Otherwise it is
        stored as 1.
        """
        index = (report.get_version(), os.path.basename(report.get_name()), report.get_power_budget(),
                 report.get_tree_decider(), report.get_leaf_decider(), report.get_node_name())

        if index not in self._reports.keys():
            self._reports[index] = 1
        else:
            self._reports[index] += 1

    def _get_base_index(self, report):
        """
        Takes a report as input, and returns the unique tuple to identify this report in the DataFrame.  Note that
        this method appends the current experiment iteration to the end of the returned tuple.  E.g.:
        >>> self._index_tracker.get_base_index(rr)
        ('0.1.1+dev365gfcda929', 'geopm_test_integration', 170, 'static_policy', 'power_balancing', 'mr-fusion2', 1)
        """
        key = (report.get_version(), os.path.basename(report.get_name()), report.get_power_budget(),
               report.get_tree_decider(), report.get_leaf_decider(), report.get_node_name())

        return key + (self._reports[key], )

    def get_multiindex(self, report):
        """
        Returns a multiindex from this report.  Used in DataFrame construction.
        """
        self._check_increment(report)

        itl = []
        for region in sorted(report.keys()):   # Pandas sorts the keys when a DF is created
            itl += [self._get_base_index(report) + (region, )] # Append region to the existing tuple
        mi = pandas.MultiIndex.from_tuples(itl, names=['version', 'name', 'power_budget', 'tree_decider',
                                                       'leaf_decider', 'node_name', 'iteration', 'region'])
        return mi


class Report(dict):
    _version = None
    _name = None
    _mode = None
    _tree_decider = None
    _leaf_decider = None
    _power_budget = None

    @staticmethod
    def reset_vars():
        (Report._version, Report._name, Report._mode, Report._tree_decider, Report._leaf_decider, Report._power_budget) = \
            None, None, None, None, None, None

    def __init__(self, report_path, offset=0):
        super(Report, self).__init__()
        self._path = report_path
        self._offset = offset
        self._version = None
        self._name = None
        self._mode = None
        self._tree_decider = None
        self._leaf_decider = None
        self._power_budget = None
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
                elif self._name is None:
                    match = re.search(r'^Profile: (\S+)$', line)
                    if match is not None:
                        self._name = match.group(1)
                elif self._mode is None:
                    match = re.search(r'^Policy Mode: (\S+)$', line)
                    if match is not None:
                        self._mode = match.group(1)
                elif self._tree_decider is None:
                    match = re.search(r'^Tree Decider: (\S+)$', line)
                    if match is not None:
                        self._tree_decider = match.group(1)
                elif self._leaf_decider is None:
                    match = re.search(r'^Leaf Decider: (\S+)$', line)
                    if match is not None:
                        self._leaf_decider = match.group(1)
                elif self._power_budget is None:
                    match = re.search(r'^Power Budget: (\S+)$', line)
                    if match is not None:
                        self._power_budget = int(match.group(1))
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
                        (region_name, region_id, runtime, energy, frequency, mpi_runtime, count) = \
                            None, None, None, None, None, None, None
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

        # Check static vars to see if they were parsed.  if not, use the Report vals
        if self._version is None and Report._version:
            self._version = Report._version
        elif self._version:
            Report._version = self._version
        else:
            raise SyntaxError('Unable to parse version information from report!')
        if self._name is None and Report._name:
            self._name = Report._name
        elif self._name:
            Report._name = self._name
        else:
            raise SyntaxError('Unable to parse name information from report!')
        if self._mode is None and Report._mode:
            self._mode = Report._mode
        elif self._mode:
            Report._mode = self._mode
        else:
            raise SyntaxError('Unable to parse mode information from report!')
        if self._tree_decider is None and Report._tree_decider:
            self._tree_decider = Report._tree_decider
        elif self._tree_decider:
            Report._tree_decider = self._tree_decider
        else:
            raise SyntaxError('Unable to parse tree_decider information from report!')
        if self._leaf_decider is None and Report._leaf_decider:
            self._leaf_decider = Report._leaf_decider
        elif self._leaf_decider:
            Report._leaf_decider = self._leaf_decider
        else:
            raise SyntaxError('Unable to parse leaf_decider information from report!')
        if self._power_budget is None and Report._power_budget:
            self._power_budget = Report._power_budget
        elif self._power_budget:
            Report._power_budget = self._power_budget
        else:
            raise SyntaxError('Unable to parse power_budget information from report!')

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

    def get_mode(self):
        return self._mode

    def get_tree_decider(self):
        return self._tree_decider

    def get_leaf_decider(self):
        return self._leaf_decider

    def get_power_budget(self):
        return self._power_budget


class Region(dict):
    def __init__(self, name, rid, runtime, energy, frequency, mpi_runtime, count):
        super(Region, self).__init__()
        self['name'] = name
        self['id'] = rid
        self['runtime'] = float(runtime)
        self['energy'] = float(energy)
        self['frequency'] = float(frequency)
        self['mpi_runtime'] = float(mpi_runtime)
        self['count'] = int(count)

    def __repr__(self):
        template = """\
{name} ({rid})
  runtime     : {runtime}
  energy      : {energy}
  frequency   : {frequency}
  mpi-runtime : {mpi_runtime}
  count       : {count}
"""
        return template.format(name=self['name'],
                               rid=self['id'],
                               runtime=self['runtime'],
                               energy=self['energy'],
                               frequency=self['frequency'],
                               mpi_runtime=self['mpi_runtime'],
                               count=self['count'])

    def __str__(self):
        return self.__repr__()

    def get_name(self):
        return self['name']

    def get_id(self):
        return self['id']

    def get_runtime(self):
        return self['runtime']

    def get_energy(self):
        return self['energy']

    def get_frequency(self):
        return self['frequency']

    def get_mpi_runtime(self):
        return self['mpi_runtime']

    def get_count(self):
        return self['count']


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

