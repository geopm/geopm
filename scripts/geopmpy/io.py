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
"""
GEOPM IO - Helper module for parsing/processing report and trace files.
"""

from __future__ import absolute_import
from __future__ import division

from builtins import str
from collections import OrderedDict
import os
import json
import re
import pandas
import numpy
import glob
import sys
import subprocess
import psutil
import copy
import tempfile
import yaml
import io

from distutils.spawn import find_executable
from natsort import natsorted
from geopmpy import __version__
from geopmpy import agent

try:
    _, os.environ['COLUMNS'] = subprocess.check_output(['stty', 'size']).decode().split()
except subprocess.CalledProcessError:
    os.environ['COLUMNS'] = "200"

pandas.set_option('display.width', int(os.environ['COLUMNS']))
pandas.set_option('display.max_colwidth', 80)
pandas.set_option('max_columns', 100)


class AppOutput(object):
    """The container class for all report and trace related data.

    This class holds the relevant objects for parsing and indexing all
    data that is output from GEOPM.  This object can be created with a
    report glob string, a trace glob string, or both that will be used
    to search dir_name for the relevant files.  If files are found
    their data will be parsed into objects for easy data access.
    Additionally a Pandas DataFrame is constructed containing all of
    the report data and a separate DataFrame containing all of the
    trace data.  These DataFrames are indexed based on the version of
    GEOPM found in the files, the profile name, agent name, and the number
    of times that particular configuration has been seen by the parser
    (i.e. experiment iteration).

    Attributes:
        report_glob: The string pattern to use to search for report files.
        trace_glob: The string pattern to use to search for trace files.
        dir_name: The directory path to use when searching for files.
        verbose: A bool to control whether verbose output is printed to stdout.

    """

    def __init__(self, reports=None, traces=None, dir_name='.', verbose=False, do_cache=True):
        self._reports = {}
        self._reports_df = pandas.DataFrame()
        self._traces = {}
        self._traces_df = pandas.DataFrame()
        self._all_paths = []
        self._index_tracker = IndexTracker()
        self._node_names = None
        self._region_names = None

        if reports:
            if type(reports) is list:
                report_paths = [os.path.join(dir_name, path) for path in reports]
            else:
                report_glob = os.path.join(dir_name, reports)
                try:
                    report_paths = glob.glob(report_glob)
                except TypeError:
                    raise TypeError('<geopm> geopmpy.io: AppOutput: reports must be a list of paths or a glob pattern')
                report_paths = natsorted(report_paths)
                if len(report_paths) == 0:
                    raise RuntimeError('<geopm> geopmpy.io: No report files found with pattern {}.'.format(report_glob))

            self._all_paths.extend(report_paths)

            if do_cache:
                paths_str = str(report_paths)
                report_h5_name = 'report_{}.h5'.format(hash(paths_str))
                self._all_paths.append(report_h5_name)

                # check if cache is older than reports
                if os.path.exists(report_h5_name):
                    cache_mod_time = os.path.getmtime(report_h5_name)
                    regen_cache = False
                    for report_file in report_paths:
                        mod_time = os.path.getmtime(report_file)
                        if mod_time > cache_mod_time:
                            regen_cache = True
                    if regen_cache:
                        os.remove(report_h5_name)

                try:
                    # load dataframes from cache
                    self._reports_df = pandas.read_hdf(report_h5_name, 'report')
                    self._app_reports_df = pandas.read_hdf(report_h5_name, 'app_report')
                    if verbose:
                        sys.stdout.write('Loaded reports from {}.\n'.format(report_h5_name))
                except IOError as err:
                    sys.stderr.write('Warning: <geopm> geopmpy.io: Report HDF5 file not detected or older than reports.  Data will be saved to {}.\n'
                                     .format(report_h5_name))
                    self.parse_reports(report_paths, verbose)

                    # Cache report dataframe
                    try:
                        if verbose:
                            sys.stdout.write('Generating HDF5 files... ')
                        self._reports_df.to_hdf(report_h5_name, 'report', format='table')
                        self._app_reports_df.to_hdf(report_h5_name, 'app_report', format='table', append=True)
                    except ImportError as error:
                        sys.stderr.write('Warning: <geopm> geopmpy.io: Unable to write HDF5 file: {}\n'.format(str(error)))

                    if verbose:
                        sys.stdout.write('Done.\n')
                        sys.stdout.flush()
            else:
                self.parse_reports(report_paths, verbose)

        if traces:
            if type(traces) is list:
                trace_paths = [os.path.join(dir_name, path) for path in traces]
            else:
                trace_glob = os.path.join(dir_name, traces)
                try:
                    trace_paths = glob.glob(trace_glob)
                except TypeError:
                    raise TypeError('<geopm> geopmpy.io: AppOutput: traces must be a list of paths or a glob pattern')
                trace_paths = natsorted(trace_paths)
                if len(trace_paths) == 0:
                    raise RuntimeError('<geopm> geopmpy.io: No trace files found with pattern {}.'.format(trace_glob))

            self._all_paths.extend(trace_paths)
            self._index_tracker.reset()

            if do_cache:
                # unique cache name based on trace files in this list
                paths_str = str(trace_paths)
                trace_h5_name = 'trace_{}.h5'.format(hash(paths_str))
                self._all_paths.append(trace_h5_name)

                # check if cache is older than traces
                if os.path.exists(trace_h5_name):
                    cache_mod_time = os.path.getmtime(trace_h5_name)
                    regen_cache = False
                    for trace_file in trace_paths:
                        mod_time = os.path.getmtime(trace_file)
                        if mod_time > cache_mod_time:
                            regen_cache = True
                    if regen_cache:
                        os.remove(trace_h5_name)

                try:
                    self._traces_df = pandas.read_hdf(trace_h5_name, 'trace')
                    if verbose:
                        sys.stdout.write('Loaded traces from {}.\n'.format(trace_h5_name))
                except IOError as err:
                    sys.stderr.write('Warning: <geopm> geopmpy.io: Trace HDF5 file not detected or older than traces.  Data will be saved to {}.\n'
                                     .format(trace_h5_name))

                    self.parse_traces(trace_paths, verbose)
                    # Cache traces dataframe
                    try:
                        if verbose:
                            sys.stdout.write('Generating HDF5 files... ')
                        self._traces_df.to_hdf(trace_h5_name, 'trace')
                    except ImportError as error:
                        sys.stderr.write('Warning: <geopm> geopmpy.io: Unable to write HDF5 file: {}\n'.format(str(error)))

                    if verbose:
                        sys.stdout.write('Done.\n')
                        sys.stdout.flush()
            else:
                self.parse_traces(trace_paths, verbose)

    def parse_reports(self, report_paths, verbose):
        reports_df_list = []
        reports_app_df_list = []
        files = 0
        filesize = 0
        for rp in report_paths:  # Get report count for verbose progress
            filesize += os.stat(rp).st_size
            with open(rp, 'r') as fid:
                for line in fid:
                    if re.findall(r'Host:', line):
                        files += 1

        filesize = '{}KiB'.format(filesize // 1024)
        fileno = 1
        for rp in report_paths:
            # Parse the first report
            rr_size = os.stat(rp).st_size
            rr = Report(rp)
            if verbose:
                sys.stdout.write('\rParsing report {} of {} ({})... '.format(fileno, files, filesize))
                sys.stdout.flush()
            fileno += 1
            self.add_report_df(rr, reports_df_list, reports_app_df_list)
            # Parse the remaining reports in this file
            while (rr.get_last_offset() != rr_size):
                rr = Report(rp, rr.get_last_offset())
                if rr.get_node_name() is not None:
                    self.add_report_df(rr, reports_df_list, reports_app_df_list)
                    if verbose:
                        sys.stdout.write('\rParsing report {} of {} ({})... '.format(fileno, files, filesize))
                        sys.stdout.flush()
                    fileno += 1
            Report.reset_vars()  # If we've reached the end of a report, reset the static vars
        if verbose:
            sys.stdout.write('Done.\n')
            sys.stdout.flush()

        if verbose:
            sys.stdout.write('Creating combined reports DF... ')
            sys.stdout.flush()
        self._reports_df = pandas.concat(reports_df_list)
        self._reports_df = self._reports_df.sort_index(ascending=True)
        self._app_reports_df = pandas.concat(reports_app_df_list)
        self._app_reports_df = self._app_reports_df.sort_index(ascending=True)
        if verbose:
            sys.stdout.write('Done.\n')
            sys.stdout.flush()

    def parse_traces(self, trace_paths, verbose):
        traces_df_list = []
        fileno = 1
        filesize = 0
        for tp in trace_paths:  # Get size of all trace files
            filesize += os.stat(tp).st_size
        # Abort if traces are too large
        avail_mem = psutil.virtual_memory().available
        if filesize > avail_mem // 2:
            sys.stderr.write('Warning: <geopm> geopmpy.io: Total size of traces is greater than 50% of available memory. Parsing traces will be skipped.\n')
            return

        filesize = '{}MiB'.format(filesize // 1024 // 1024)

        for tp in trace_paths:
            if verbose:
                sys.stdout.write('\rParsing trace file {} of {} ({})... '.format(fileno, len(trace_paths), filesize))
                sys.stdout.flush()
            fileno += 1
            tt = Trace(tp)
            self.add_trace_df(tt, traces_df_list)  # Handles multiple traces per node
        if verbose:
            sys.stdout.write('Done.\n')
            sys.stdout.flush()
        if verbose:
            sys.stdout.write('Creating combined traces DF... ')
            sys.stdout.flush()
        self._traces_df = pandas.concat(traces_df_list)
        self._traces_df = self._traces_df.sort_index(ascending=True)
        if verbose:
            sys.stdout.write('Done.\n')
            sys.stdout.flush()

    def remove_files(self):
        """Deletes all files currently tracked by this object."""
        for ff in self._all_paths:
            try:
                os.remove(ff)
            except OSError:
                pass

    def add_report_df(self, rr, reports_df_list, reports_app_df_list):
        """Adds a report DataFrame to the tracking list.

        The report tracking list is used to create the combined
        DataFrame once all reports are parsed.

        Args:
            rr: The report object that will be converted to a
                DataFrame, indexed, and added to the tracking list.

        """
        # Build and index the DF
        rdf = pandas.DataFrame(rr).T.drop('name', 1)
        numeric_cols = ['count', 'energy_pkg', 'energy_dram', 'frequency', 'network_time', 'runtime', 'sync_runtime']
        rdf[numeric_cols] = rdf[numeric_cols].apply(pandas.to_numeric)

        # Add extra index info
        index = self._index_tracker.get_multiindex(rr)
        rdf = rdf.set_index(index)
        reports_df_list.append(rdf)

        # Save application totals
        app = {'runtime': rr.get_runtime(),
               'energy-package': rr.get_energy_pkg(),
               'energy-dram': rr.get_energy_dram(),
               'network-time': rr.get_network_time(),
               'ignore-runtime': rr.get_ignore_runtime(),
               'memory-hwm': rr.get_memory_hwm(),
               'network-bw': rr.get_network_bw()
              }
        index = index.droplevel('region').drop_duplicates()
        app_df = pandas.DataFrame(app, index=index)
        numeric_cols = list(app)
        app_df[numeric_cols] = app_df[numeric_cols].apply(pandas.to_numeric)
        reports_app_df_list.append(app_df)

    def add_trace_df(self, tt, traces_df_list):
        """Adds a trace DataFrame to the tracking list.

        The report tracking list is used to create the combined
        DataFrame once all reports are parsed.

        Args:
            tt: The Trace object used to extract the Trace DataFrame.
                This DataFrame will be indexed and added to the
                tracking list.

        """
        tdf = tt.get_df()  # TODO: this needs numeric cols optimization
        tdf = tdf.set_index(self._index_tracker.get_multiindex(tt))
        traces_df_list.append(tdf)

    def get_node_names(self):
        """Returns the names of the nodes detected in the parse report files.

        Note that this is only useful for a single experiment's
        dataset.  The _reports dictionary is populated from every
        report file that was globbed, so if you have multiple
        iterations of an experiment the last set of reports parsed
        will be contained in this dictionary.  Additionally, if
        different nodes were used with different experiment iterations
        then this dictionary will not have consistent data.

        If analysis of all of the data is desired, use get_report_df()
        to get a combined DataFrame of all the data.

        """
        if self._node_names is None:
            self._node_names = self._reports_df.index.get_level_values('node_name').unique().tolist()
        return self._node_names

    def get_region_names(self):
        if self._region_names is None:
            self._region_names = self._reports_df.index.get_level_values('region').unique().tolist()
        return self._region_names

    def get_report_data(self, profile=None, agent=None, node_name=None, region=None):
        idx = pandas.IndexSlice
        df = self._reports_df
        if profile is not None:
            if type(profile) is tuple:
                minp, maxp = profile
                df = df.loc[idx[:, :, minp:maxp, :, :, :, :], ]
            else:
                df = df.loc[idx[:, :, profile, :, :, :, :], ]
        if agent is not None:
            df = df.loc[idx[:, :, :, agent, :, :, :], ]
        if node_name is not None:
            df = df.loc[idx[:, :, :, :, node_name, :, :], ]
        if region is not None:
            df = df.loc[idx[:, :, :, :, :, :, region], ]
        return df

    # TODO Call this from outside code to get totals
    def get_app_total_data(self, node_name=None):
        idx = pandas.IndexSlice
        df = self._app_reports_df
        if node_name is not None:
            df = df.loc[idx[:, :, :, :, node_name, :], ]
        return df

    def get_trace_data(self, node_name=None):
        idx = pandas.IndexSlice
        df = self._traces_df
        if node_name is not None:
            df = df.loc[idx[:, :, :, :, node_name, :, :], ]
        return df

    def get_report_df(self):
        """Getter for the combined DataFrame of all report files parsed.

        This DataFrame contains all data parsed, and has a complex
        MultiIndex for accessing the unique data from each individual
        report.  For more information on this index, see the
        IndexTracker docstring.

        Returns:
            pandas.DataFrame: Contains all parsed data.

        """
        return self._reports_df

    def get_trace_df(self):
        """Getter for the combined DataFrame of all trace files parsed.

        This DataFrame contains all data parsed, and has a complex
        MultiIndex for accessing the unique data from each individual
        trace.  For more information on this index, see the
        IndexTracker docstring.

        Returns:
            pandas.DataFrame: Contains all parsed data.

        """
        return self._traces_df

    def extract_index_from_profile(self, inplace=False):
        """
        Pulls the power budget or other number out of the profile name
        and replaces the name column of the data frame with this
        number.
        """
        profile_name_map = {}
        names_list = self._reports_df.index.get_level_values('name').unique().tolist()
        for name in names_list:
            # The profile name is currently set to: ${NAME}_${POWER_BUDGET}
            profile_name_map.update({name: int(name.split('_')[-1])})
        df = self._reports_df.rename(index=profile_name_map).sort_index(ascending=True)
        if inplace:
            self._reports_df = df
        return df


class IndexTracker(object):
    """Tracks and uniquely identifies experiment configurations for
       DataFrame indexing.

    This object's purpose is to examine parsed data for reports or
    traces and determine if a particular experiment configuration has
    already been tracked.  A user may run the same configuration
    repeatedly in order to prove that results are repeatable and are
    not outliers.  Since the same configuration is used many times, it
    must be tracked and counted to ensure that the unique data for
    each run can be extracted later.

    The parsed data is used to extract the following fields to build
    the tracking index tuple:
        (<GEOPM_VERSION>, <PROFILE_NAME>, <AGENT_NAME>, <NODE_NAME>)

    If the tuple not contained in the _run_outputs dict, it is
    inserted with a value of 1.  The value is incremented if the tuple
    is currently in the _run_outputs dict.  This value is used to
    uniquely identify a particular set of parsed data when the
    MultiIndex is created.

    """
    def __init__(self):
        self._run_outputs = {}

    def _check_increment(self, run_output):
        """Extracts the index tuple from the parsed data and tracks it.

        Checks to see if the current run_output has been seen before.
        If so, the count is incremented.  Otherwise it is stored as 1.

        Args:
            run_output: The Report or Trace object to be tracked.
        """
        index = (run_output.get_version(), run_output.get_start_time(),
                 os.path.basename(run_output.get_profile_name()),
                 run_output.get_agent(), run_output.get_node_name())
        if index not in self._run_outputs:
            self._run_outputs[index] = 1
        else:
            self._run_outputs[index] += 1

    def _get_base_index(self, run_output):
        """Constructs the actual index tuple to be used to construct a
           uniquely-identifying MultiIndex for this data.

        Takes a run_output as input, and returns the unique tuple to
        identify this run_output in the DataFrame.  Note that this
        method appends the current experiment iteration to the end of
        the returned tuple.  E.g.:

        >>> self._index_tracker.get_base_index(rr)
        ('0.1.1+dev365gfcda929', 'geopm_test_integration', 170,
        'static_policy', 'power_balancing', 'mr-fusion2', 1)

        Args:
            run_output: The Report or Trace object to produce an index tuple for.

        Returns:
            Tuple: This will contain all of the index fields needed to uniquely identify this data (including the
                count of how many times this experiment has been seen.

        """
        key = (run_output.get_version(), run_output.get_start_time(),
               os.path.basename(run_output.get_profile_name()),
               run_output.get_agent(), run_output.get_node_name())

        return key + (self._run_outputs[key], )

    def get_multiindex(self, run_output):
        """Returns a MultiIndex from this run_output.  Used in DataFrame construction.

        This will add the current run_output to the list of tracked
        data, and return a unique muiltiindex tuple to identify this
        data in a DataFrame.

        For Report objects, the region name is appended to the end of
        the tuple.  For Trace objects, the integer index of the
        DataFrame is appended to the tuple.

        Args:
            run_output: The Report or Trace object to produce an index
                        tuple for.

        Returns:
            pandas.MultiIndex: The unique index to identify this data object.

        """
        self._check_increment(run_output)

        itl = []
        index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration']

        if type(run_output) is Report:
            index_names.append('region')
            for region in run_output:
                itl.append(self._get_base_index(run_output) + (region, ))  # Append region to the existing tuple
        else:  # Trace file index
            index_names.append('index')
            for ii in range(len(run_output.get_df())):  # Append the integer index to the DataFrame index
                itl.append(self._get_base_index(run_output) + (ii, ))

        mi = pandas.MultiIndex.from_tuples(itl, names=index_names)
        return mi

    def reset(self):
        """Clears the internal tracking dictionary.

        Since only one type of data (reports OR traces) can be tracked
        at once, this is necessary to reset the object's state so a
        new type of data can be tracked.

        """
        self._run_outputs = {}


class Report(OrderedDict):
    """An object to parse and encapsulate the data from a report file.

    Reports from the GEOPM runtime are currently coalesced into a
    single file from all the nodes used in a particular run.  This
    class will process the combined file one line at a time and
    attempt to extract and encapsulate a single report in its member
    variables.  The intention is that for a combined file, one would
    first create one of these objects to extract the first report.  To
    parse the remaining reports, you will create a new object with the
    same report_path but use the offset from the previous object to
    see where you left off in the parsing process.

    Attributes:
        report_path: A string path to a report file to be parsed.
        offset: The starting offset within the report_path file to
                begin looking for a new report.

    """
    # These variables are intentionally defined outside __init__().  They occur once at the top of a combined report
    # file and are needed for all report contained in the combined file.  Defining them this way allows the initial
    # value to be shared among all Report files created.
    _version = None
    _name = None
    _agent = None
    _start_time = None

    @staticmethod
    def reset_vars():
        """Clears the static variables used in this class.  Should be called
        just before parsing a second, third, etc.  report file since
        these fields may change.

        """
        (Report._version, Report._name, Report._agent, Report._start_time) = \
            None, None, None, None

    def __init__(self, report_path, offset=0):
        super(Report, self).__init__()
        self._path = report_path
        self._offset = offset
        self._version = None
        self._start_time = None
        self._profile_name = None
        self._agent = None
        self._total_runtime = None
        self._total_energy_pkg = None
        self._total_energy_dram = None
        self._total_ignore_runtime = None
        self._total_network_time = None
        self._total_memory_hwm = None
        self._total_network_bw = None
        self._node_name = None

        found_totals = False
        (region_name, region_id, runtime, sync_runtime, energy_pkg, energy_dram, frequency, network_time, count) = None, None, None, None, None, None, None, None, None
        float_regex = r'([-+]?(\d+(\.\d*)?|\.\d+)([eE][-+]?\d+)?)'

        with open(self._path, 'r') as fid:
            fid.seek(self._offset)
            line = fid.readline()
            while len(line) != 0:
                if self._version is None:
                    match = re.search(r'^##### geopm (\S+) #####$', line)
                    if match is not None:
                        self._version = match.group(1)
                if self._start_time is None:
                    match = re.search(r'^Start Time: (.+)$', line)
                    if match is not None:
                        self._start_time = match.group(1)
                if self._profile_name is None:
                    match = re.search(r'^Profile: (\S+)$', line)
                    if match is not None:
                        self._profile_name = match.group(1)
                if self._agent is None:
                    match = re.search(r'^Agent: (\S+)$', line)
                    if match is not None:
                        self._agent = match.group(1)
                if self._node_name is None:
                    match = re.search(r'^Host: (\S+)$', line)
                    if match is not None:
                        self._node_name = match.group(1)
                if region_name is None:
                    match = re.search(r'^Region (\S+) \((0x)?([0-9a-fA-F]+)\):', line)
                    if match is not None:
                        region_name = match.group(1)
                        if match.group(2) is None:
                            region_id = match.group(3)
                        else:
                            region_id = match.group(2) + match.group(3)
                if region_name is None:
                    match = re.search(r'^Epoch Totals:', line)
                    if match is not None:
                        region_name = 'epoch'
                if runtime is None:
                    match = re.search(r'^\s+runtime.+: ' + float_regex, line)
                    if match is not None:
                        runtime = float(match.group(1))
                if sync_runtime is None:
                    match = re.search(r'^\s+sync-runtime.+: ' + float_regex, line)
                    if match is not None:
                        sync_runtime = float(match.group(1))
                if energy_pkg is None:
                    match = re.search(r'^\s+package-energy.+: ' + float_regex, line)
                    if match is not None:
                        energy_pkg = float(match.group(1))
                if energy_dram is None:
                    match = re.search(r'^\s+dram-energy.+: ' + float_regex, line)
                    if match is not None:
                        energy_dram = float(match.group(1))
                if frequency is None:
                    match = re.search(r'^\s+frequency \(%\)+: ' + float_regex, line)
                    if match is not None:
                        frequency = float(match.group(1))
                if network_time is None:
                    match = re.search(r'^\s+network-time.+: ' + float_regex, line)
                    if match is not None:
                        network_time = float(match.group(1))
                if count is None:
                    match = re.search(r'^\s+count: ' + float_regex, line)
                    if match is not None:
                        count = float(match.group(1))
                        self[region_name] = Region(region_name, region_id, runtime, sync_runtime, energy_pkg, energy_dram, frequency, network_time, count)
                        (region_name, region_id, runtime, sync_runtime, energy_pkg, energy_dram, frequency, network_time, count) = \
                            None, None, None, None, None, None, None, None, None
                if not found_totals:
                    match = re.search(r'^Application Totals:$', line)
                    if match is not None:
                        found_totals = True
                else:
                    if self._total_runtime is None:
                        match = re.search(r'\s+runtime.+: ' + float_regex, line)
                        if match is not None:
                            self._total_runtime = float(match.group(1))
                    if self._total_energy_pkg is None:
                        match = re.search(r'\s+package-energy.+: ' + float_regex, line)
                        if match is not None:
                            self._total_energy_pkg = float(match.group(1))
                    if self._total_energy_dram is None:
                        match = re.search(r'\s+dram-energy.+: ' + float_regex, line)
                        if match is not None:
                            self._total_energy_dram = float(match.group(1))
                    if self._total_network_time is None:
                        match = re.search(r'\s+network-time.+: ' + float_regex, line)
                        if match is not None:
                            self._total_network_time = float(match.group(1))
                    if self._total_ignore_runtime is None:
                        match = re.search(r'\s+ignore-time.+: ' + float_regex, line)
                        if match is not None:
                            self._total_ignore_runtime = float(match.group(1))
                    if self._total_memory_hwm is None:
                        match = re.search(r'\s+geopmctl memory HWM: ' + float_regex + ' kB$', line)
                        if match is not None:
                            self._total_memory_hwm = float(match.group(1))
                    if self._total_network_bw is None:
                        match = re.search(r'\s+geopmctl network BW.+: ' + float_regex, line)
                        if match is not None:
                            self._total_network_bw = float(match.group(1))
                            break # End of report blob

                line = fid.readline()
            self._offset = fid.tell()

        # Check static vars to see if they were parsed.  if not, use the Report vals
        if self._version is None and Report._version:
            self._version = Report._version
        elif self._version:
            Report._version = self._version
        else:
            raise SyntaxError('<geopm> geopmpy.io: Unable to parse version information from report!')
        if self._profile_name is None and Report._profile_name:
            self._profile_name = Report._profile_name
        elif self._profile_name:
            Report._profile_name = self._profile_name
        else:
            raise SyntaxError('<geopm> geopmpy.io: Unable to parse name information from report!')
        if self._agent is None and Report._agent:
            self._agent = Report._agent
        elif self._agent:
            Report._agent = self._agent
        else:
            raise SyntaxError('<geopm> geopmpy.io: Unable to parse agent information from report!')
        if self._start_time is None and Report._start_time:
            self._start_time = Report._start_time
        elif self._start_time:
            Report._start_time = self._start_time
        else:
            raise SyntaxError('<geopm> geopmpy.io: Unable to parse start time from report!')

        # TODO: temporary hack to use old data
        if self._total_energy_dram is None:
            self._total_energy_dram = 0
        if (len(line) != 0 and (region_name is not None or not found_totals or
            None in (self._total_runtime, self._total_energy_pkg, self._total_energy_dram, self._total_ignore_runtime, self._total_network_time))):
            raise SyntaxError('<geopm> geopmpy.io: Unable to parse report {} before offset {}: '.format(self._path, self._offset))

    # Fields used for dataframe construction only
    def get_profile_name(self):
        return self._profile_name

    def get_start_time(self):
        return self._start_time

    def get_version(self):
        return self._version

    def get_agent(self):
        return self._agent

    def get_node_name(self):
        return self._node_name

    def get_last_offset(self):
        return self._offset

    # Application totals
    def get_runtime(self):
        return self._total_runtime

    def get_ignore_runtime(self):
        return self._total_ignore_runtime

    def get_network_time(self):
        return self._total_network_time

    def get_energy_pkg(self):
        return self._total_energy_pkg

    def get_energy_dram(self):
        return self._total_energy_dram

    def get_memory_hwm(self):
        return self._total_memory_hwm

    def get_network_bw(self):
        return self._total_network_bw


class Region(OrderedDict):
    """Encapsulates all data related to a region from a report file.

    Attributes:
        name: The name of the region.
        rid: The numeric ID of the region.
        runtime: The accumulated time of the region in seconds.
        energy_pkg: The accumulated package energy from this region in
                    Joules.
        energy_dram: The accumulated DRAM energy from this region in
                     Joules.
        frequency: The average frequency achieved during this region
                   in terms of percent of sticker frequency.
        network_time: The accumulated time in this region executing MPI
                     calls in seconds.
        count: The number of times this region has been entered.

    """
    def __init__(self, name, rid, runtime, sync_runtime, energy_pkg, energy_dram, frequency, network_time, count):
        super(Region, self).__init__()
        self['name'] = name
        self['id'] = rid
        self['runtime'] = float(runtime)
        self['sync_runtime'] = float(sync_runtime) if sync_runtime is not None else runtime
        self['energy_pkg'] = float(energy_pkg)
        self['energy_dram'] = float(energy_dram) if energy_dram is not None else 0
        self['frequency'] = float(frequency)
        self['network_time'] = float(network_time)
        self['count'] = int(count)

    def __repr__(self):
        template = """\
{name} ({rid})
  runtime     : {runtime}
  sync-runtime : {sync_runtime}
  package-energy : {energy_pkg}
  dram-energy : {energy_dram}
  frequency   : {frequency}
  network-time : {network_time}
  count       : {count}
"""
        return template.format(name=self['name'],
                               rid=self['id'],
                               runtime=self['runtime'],
                               sync_runtime=self['sync_runtime'],
                               energy_pkg=self['energy_pkg'],
                               energy_dram=self['energy_dram'],
                               frequency=self['frequency'],
                               network_time=self['network_time'],
                               count=self['count'])

    def __str__(self):
        return self.__repr__()

    def get_name(self):
        return self['name']

    def get_id(self):
        return self['id']

    def get_runtime(self):
        return self['runtime']

    def get_sync_runtime(self):
        return self['sync_runtime']

    def get_energy_pkg(self):
        return self['energy_pkg']

    def get_energy_dram(self):
        return self['energy_dram']

    def get_frequency(self):
        return self['frequency']

    def get_network_time(self):
        return self['network_time']

    def get_count(self):
        return self['count']


class Trace(object):
    """Creates a pandas DataFrame comprised of the trace file data.

    This object will parse both the header and the CSV data in a trace
    file.  The header identifies the uniquely-identifying configuration
    for this file which is used for later indexing purposes.

    Even though __getattr__() and __getitem__() allow this object to
    effectively be treated like a DataFrame, you must use get_df() if
    you're building a list of DataFrames to pass to pandas.concat().
    Using the raw object in a list and calling concat will cause an
    error.

    Attributes:
        trace_path: The path to the trace file to parse.
    """
    def __init__(self, trace_path, use_agent=True):
        self._path = trace_path

        old_headers = {'time': 'TIME',
                       'epoch_count': 'EPOCH_COUNT',
                       'region_hash': 'REGION_HASH',
                       'region_hint': 'REGION_HINT',
                       'region_progress': 'REGION_PROGRESS',
                       'region_count': 'REGION_COUNT',
                       'region_runtime': 'REGION_RUNTIME',
                       'energy_package': 'ENERGY_PACKAGE',
                       'energy_dram': 'ENERGY_DRAM',
                       'power_package': 'POWER_PACKAGE',
                       'power_dram': 'POWER_DRAM',
                       'frequency': 'FREQUENCY',
                       'cycles_thread': 'CYCLES_THREAD',
                       'cycles_reference': 'CYCLES_REFERENCE',
                       'temperature_core': 'TEMPERATURE_CORE'}

        old_balancer_headers = {'policy_power_cap': 'POLICY_POWER_CAP',
                                'policy_step_count': 'POLICY_STEP_COUNT',
                                'policy_max_epoch_runtime': 'POLICY_MAX_EPOCH_RUNTIME',
                                'policy_power_slack': 'POLICY_POWER_SLACK',
                                'epoch_runtime': 'EPOCH_RUNTIME',
                                'power_limit': 'POWER_LIMIT',
                                'enforced_power_limit': 'ENFORCED_POWER_LIMIT'}
        old_headers.update(old_balancer_headers)

        old_governor_headers = {'power_budget': 'POWER_BUDGET'}
        old_headers.update(old_governor_headers)

        column_headers = pandas.read_csv(trace_path, sep='|', comment='#', nrows=0, encoding='utf-8').columns.tolist()
        original_headers = copy.deepcopy(column_headers)

        column_headers = [old_headers.get(ii, ii) for ii in column_headers]

        if column_headers != original_headers:
            sys.stderr.write('Warning: <geopm> geopmpy.io: Old trace file format detected. Old column headers will be forced ' \
                             'to UPPERCASE.\n')

        # region_hash and region_hint must be a string for pretty printing pandas DataFrames
        # You can force them to int64 by setting up a converter function then passing the hex string through it
        # with the read_csv call, but the number will be displayed as an integer from then on.  You'd have to convert
        # it back to a hex string to compare it with the data in the reports.
        self._df = pandas.read_csv(trace_path, sep='|', comment='#', header=0, names=column_headers, encoding='utf-8',
                                   dtype={'REGION_HASH': 'unicode', 'REGION_HINT': 'unicode'})
        self._df.columns = list(map(str.strip, self._df[:0]))  # Strip whitespace from column names
        self._df['REGION_HASH'] = self._df['REGION_HASH'].astype('unicode').map(str.strip)  # Strip whitespace from region hashes
        self._df['REGION_HINT'] = self._df['REGION_HINT'].astype('unicode').map(str.strip)  # Strip whitespace from region hints

        self._version = None
        self._start_time = None
        self._profile_name = None
        self._agent = None
        self._node_name = None
        self._use_agent = use_agent
        self._parse_header(trace_path)

    def __repr__(self):
        return self._df.__repr__()

    def __str__(self):
        return self.__repr__()

    def __getattr__(self, attr):
        """Pass through attribute requests to the underlying DataFrame.

        This allows for Trace objects to be treated like DataFrames
        for analysis.  You can do things like:

        >>> tt = geopmpy.io.Trace('170-4-balanced-minife-trace-mr-fusion5')
        >>> tt.keys()
        Index([u'region_hash', u'region_hint', u'seconds', u'pkg_energy-0', u'dram_energy-0',...

        """
        return getattr(self._df, attr)

    def __getitem__(self, key):
        """Pass through item requests to the underlying DataFrame.

        This allows standard DataFrame slicing operations to take place.

        @todo, update
        >>> tt[['region_hash', 'region_hint', 'time', 'energy_package', 'energy_dram']][:5]
                     region_hash region_hint      time  energy_package-0  energy_dram-0
        0  2305843009213693952  0.662906     106012.363770   25631.015519
        1  2305843009213693952  0.667854     106012.873718   25631.045777
        2  2305843009213693952  0.672882     106013.411621   25631.075807
        3  2305843009213693952  0.677869     106013.998108   25631.105882
        4  2305843009213693952  0.682849     106014.621704   25631.136186
        """
        return self._df.__getitem__(key)

    def _parse_header(self, trace_path):
        """Parses the configuration header out of the top of the trace file.

        Args:
            trace_path: The path to the trace file to parse.
        """
        done = False
        out = []
        with open(trace_path) as fid:
            while not done:
                ll = fid.readline()
                if ll.startswith('#'):
                    out.append(ll[1:])
                else:
                    done = True
        try:
            yaml_fd = io.StringIO(u''.join(out))
            dd = yaml.load(yaml_fd, Loader=yaml.SafeLoader)
        except yaml.parser.ParserError:
            out.insert(0, '{')
            out.append('}')
            json_str = ''.join(out)
            dd = json.loads(json_str)
        try:
            self._version = dd['geopm_version']
            self._start_time = dd['start_time']
            self._profile_name = dd['profile_name']
            if self._use_agent:
                self._agent = dd['agent']
            self._node_name = dd['node_name']
        except KeyError:
            raise SyntaxError('<geopm> geopmpy.io: Trace file header could not be parsed!')

    def get_df(self):
        return self._df

    def get_version(self):
        return self._version

    def get_start_time(self):
        return self._start_time

    def get_profile_name(self):
        return self._profile_name

    def get_agent(self):
        return self._agent

    def get_node_name(self):
        return self._node_name

    @staticmethod
    def diff_df(trace_df, column_regex, epoch=True):
        """Diff the DataFrame.

        Since the counters in the trace files are monotonically
        increasing, a diff must be performed to extract the useful
        data.

        Args:
            trace_df: The MultiIndexed DataFrame created by the
                      AppOutput class.
            column_regex: A string representing the regex search
                          pattern for the column names to diff.
            epoch: A flag to set whether or not to focus solely on
                   epoch regions.

        Returns:

            pandas.DataFrame: With the diffed columns specified by
                              'column_regex', and an 'elapsed_time'
                              column.

        Todo:
            * Should I drop everything before the first epoch if
              'epoch' is false?

        """
        # drop_duplicates() is a workaround for #662. Duplicate data
        # rows are showing up in the trace for unmarked.
        tmp_df = trace_df.drop_duplicates()

        filtered_df = tmp_df.filter(regex=column_regex).copy()
        filtered_df['elapsed_time'] = tmp_df['time']
        if epoch:
            filtered_df['epoch_count'] = tmp_df['epoch_count']
        filtered_df = filtered_df.diff()
        # The following drops all 0's and the negative sample when traversing between 2 trace files.
        # If the epoch_count column is included, this will also drop rows occuring mid-epoch.
        filtered_df = filtered_df.loc[(filtered_df > 0).all(axis=1)]

        # Reset 'index' to be 0 to the length of the unique trace files
        traces_list = []
        for (version, start_time, name, agent, node_name, iteration), df in \
            filtered_df.groupby(level=['version', 'start_time', 'name', 'agent', 'node_name', 'iteration']):
            df = df.reset_index(level='index')
            df['index'] = pandas.Series(numpy.arange(len(df)), index=df.index)
            df = df.set_index('index', append=True)
            traces_list.append(df)

        return pandas.concat(traces_list)

    @staticmethod
    def get_median_df(trace_df, column_regex, config):
        """Extract the median experiment iteration.

        This logic calculates the sum of elapsed times for all of the
        experiment iterations for all nodes in that iteration.  It
        then extracts the DataFrame for the iteration that is closest
        to the median.  For input DataFrames with a single iteration,
        the single iteration is returned.

        Args:
            trace_df: The MultiIndexed DataFrame created by the
                      AppOutput class.
            column_regex: A string representing the regex search
                          pattern for the column names to diff.
            config: The TraceConfig object being used presently.

        Returns:
            pandas.DataFrame: Containing a single experiment iteration.

        """
        diffed_trace_df = Trace.diff_df(trace_df, column_regex, config.epoch_only)

        idx = pandas.IndexSlice
        et_sums = diffed_trace_df.groupby(level=['iteration'])['elapsed_time'].sum()
        median_index = (et_sums - et_sums.median()).abs().sort_values().index[0]
        median_df = diffed_trace_df.loc[idx[:, :, :, :, :, median_index], ]
        if config.verbose:
            median_df_index = []
            median_df_index.append(median_df.index.get_level_values('version').unique()[0])
            median_df_index.append(median_df.index.get_level_values('start_time').unique()[0])
            median_df_index.append(median_df.index.get_level_values('name').unique()[0])
            median_df_index.append(median_df.index.get_level_values('agent').unique()[0])
            median_df_index.append(median_df.index.get_level_values('iteration').unique()[0])
            sys.stdout.write('Median DF index = ({})...\n'.format(' '.join(str(s) for s in median_df_index)))
            sys.stdout.flush()
        return median_df


class BenchConf(object):
    """The application configuration parameters.

    Used to hold the config data for the integration test application.
    This application allows for varying combinations of regions
    (compute, IO, or network bound), complexity, desired execution
    count, and amount of imbalance between nodes during execution.

    Attributes:
        path: The output path for this configuration file.

    """
    def __init__(self, path):
        self._path = path
        self._loop_count = 1
        self._region = []
        self._big_o = []
        self._hostname = []
        self._imbalance = []

    def __repr__(self):
        template = """\
path      : {path}
regions   : {regions}
big-o     : {big_o}
loop count: {loops}
hostnames : {hosts}
imbalance : {imbalance}
"""
        return template.format(path=self._path,
                               regions=self._region,
                               big_o=self._big_o,
                               loops=self._loop_count,
                               hosts=self._hostname,
                               imbalance=self._imbalance)

    def __str__(self):
        return self.__repr__()

    def set_loop_count(self, loop_count):
        self._loop_count = loop_count

    def append_region(self, name, big_o):
        """Appends a region to the internal list.

        Args:
            name: The string representation of the region.
            big_o: The desired complexity of the region.  This
                   affects compute, IO, or network complexity
                   depending on the type of region requested.

        """
        self._region.append(name)
        self._big_o.append(big_o)

    def append_imbalance(self, hostname, imbalance):
        """Appends imbalance to the config for a particular node.

        Args:
            hostname: The name of the node.

            imbalance: The amount of imbalance to apply to the node.
                       This is specified by a float in the range
                       [0,1].  For example, specifying a value of 0.25
                       means that this node will spend 25% more time
                       executing the work than a node would by
                       default.  Nodes not specified with imbalance
                       configurations will perform normally.

        """
        self._hostname.append(hostname)
        self._imbalance.append(imbalance)

    def get_path(self):
        return self._path

    def write(self):
        """Write the current config to a file."""
        obj = {'loop-count': self._loop_count,
               'region': self._region,
               'big-o': self._big_o}

        if (self._imbalance and self._hostname):
            obj['imbalance'] = self._imbalance
            obj['hostname'] = self._hostname

        with open(self._path, 'w') as fid:
            json.dump(obj, fid)

    def get_exec_path(self):
        # Using libtool causes sporadic issues with the Intel
        # toolchain.
        result = 'geopmbench'
        path = find_executable(result)
        source_dir = os.path.dirname(
                     os.path.dirname(
                     os.path.dirname(
                     os.path.realpath(__file__))))
        source_bin = os.path.join(source_dir, '.libs', 'geopmbench')
        if not path:
            result = source_bin
        else:
            with open(path, 'rb') as fid:
                buffer = fid.read(4096)
                if b'Generated by libtool' in buffer:
                    result = source_bin
        return result

    def get_exec_args(self):
        return [self._path]


class AgentConf(object):
    """The GEOPM agent configuration parameters.

    This class contains all the parameters necessary to run the GEOPM
    agent with a workload.

    Attributes:
        path: The output path for this configuration file.
        options: A dict of the options for this agent.

    """
    def __init__(self, path, agent='monitor', options=dict()):
        supported_agents = {'monitor', 'power_governor', 'power_balancer', 'energy_efficient',
                            'frequency_map'}
        self._path = path
        if agent not in supported_agents:
            raise SyntaxError('<geopm> geopmpy.io: AgentConf does not support agent type: ' + agent + '!')
        self._agent = agent
        self._options = options

    def __repr__(self):
        return json.dumps(self._options)

    def __str__(self):
        return self.__repr__()

    def get_path(self):
        return self._path

    def get_agent(self):
        return self._agent

    def write(self):
        """Write the current config to a file."""
        policy_names = agent.policy_names(self._agent)
        name_offsets = { name: offset for offset, name in enumerate(policy_names)}
        policy_values = [float('nan')] * len(name_offsets)

        # Earlier versions of this function had special handling per agent instead
        # of using the agent's policy names. Translate the old-style inputs to
        # use the new style for backward compatibility.
        old_names = []
        if self._agent in ['power_governor', 'power_balancer']:
            old_names = ['power_budget']
        elif self._agent in ['frequency_map', 'energy_efficient']:
            old_names = ['frequency_min', 'frequency_max']
        policy_dict = self._options.copy()
        for offset, name in enumerate(old_names):
            if name in policy_dict:
                policy_dict[policy_names[offset]] = policy_dict.pop(name)

        for (policy_name, policy_value) in policy_dict.items():
            if policy_name not in name_offsets:
                raise KeyError('<geopm> geopmpy.io: Policy "{}" does not exist in agent "{}"'.format(policy_name, self._agent))
            policy_offset = name_offsets[policy_name]
            policy_values[policy_offset] = policy_value

        with open(self._path, "w") as outfile:
            outfile.write(agent.policy_json(self._agent, policy_values))

class RawReport(object):
    def __init__(self, path):
        with open(path) as in_fid, tempfile.TemporaryFile(mode='w+t') as out_fid:
            out_fid.write('GEOPM Meta Data:\n')
            line = in_fid.readline()
            version = line.split()[2]
            out_fid.write('    GEOPM Version: {}\n'.format(version))
            line = in_fid.readline()
            col_pos = line.find(': ')
            out_fid.write("    {}: '{}'\n".format(line[:col_pos], line[col_pos+2:-1]))
            for idx in range(2):
                line = in_fid.readline()
                out_fid.write('    {}'.format(line))
            for line in in_fid.readlines():
                if len(line) > 1:
                    if line.startswith('Host:'):
                        host = line.split(':')[1].strip()
                        out_fid.write('{}:\n'.format(host))
                    else:
                        out_fid.write('    {}'.format(line))
            out_fid.seek(0)
            self._raw_dict = yaml.load(out_fid, Loader=yaml.SafeLoader)

    def raw_report(self):
        return copy.deepcopy(self._raw_dict)

    def dump_json(self, path):
        jdata = json.dumps(self._raw_dict)
        with open(path, 'w') as fid:
            fid.write(jdata)

    def meta_data(self):
        return copy.deepcopy(self._raw_dict['GEOPM Meta Data'])

    def host_names(self):
        return [xx for xx in self._raw_dict if xx != 'GEOPM Meta Data']

    def region_names(self, host_name):
        host_data = self._raw_dict[host_name]
        result = [xx.split()[1] for xx in host_data if xx.startswith('Region ')]
        return result

    def region_hash(self, region_name):
        for host_name in self.host_names():
            host_data = self._raw_dict[host_name]
            for xx in host_data:
                if xx.startswith('Region {}'.format(region_name)):
                    return xx.split()[2][1:-1]
        raise KeyError('<geopm> geopmpy.io: Region not found: {}'.format(region_name))

    def raw_region(self, host_name, region_name):
        host_data = self._raw_dict[host_name]
        region_hash = self.region_hash(region_name)
        key = 'Region {} ({})'.format(region_name, region_hash)
        return copy.deepcopy(host_data[key])

    def raw_epoch(self, host_name):
        host_data = self._raw_dict[host_name]
        key = 'Epoch Totals'
        return copy.deepcopy(host_data[key])

    def raw_totals(self, host_name):
        host_data = self._raw_dict[host_name]
        key = 'Application Totals'
        return copy.deepcopy(host_data[key])

    def get_field(self, raw_data, key, units=''):
        matches = [(len(kk), kk) for kk in raw_data if key in kk and units in kk]
        if len(matches) == 0:
            raise KeyError('<geopm> geopmpy.io: Field not found: {}'.format(key))
        match = sorted(matches)[0][1]
        return copy.deepcopy(raw_data[match])

