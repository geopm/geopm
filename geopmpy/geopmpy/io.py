#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
"""
GEOPM IO - Helper module for parsing/processing report and trace files.
"""


from builtins import str
from collections import OrderedDict
import os
import json
import re
import pandas
import numpy
import glob
import sys
import subprocess # nosec
import psutil
import copy
import yaml
import io
import hashlib
if os.getenv('GEOPM_USE_UNSAFE_HDF5') is not None:
    from pandas import read_hdf
else:
    def read_hdf(path, name):
        if not os.path.exists(path):
            raise IOError(f'Trace HDF5 file {path} not detected')
        raise ImportWarning('Refusing to read HDF5 format files: file format could result in arbitrary code execution. To enable "export GEOPM_USE_UNSAFE_HDF5=1"')
from distutils.spawn import find_executable
from natsort import natsorted
from . import __version__

try:
    _, os.environ['COLUMNS'] = subprocess.check_output(['stty', 'size']).decode().split()
except subprocess.CalledProcessError:
    os.environ['COLUMNS'] = "200"

pandas.set_option('display.width', int(os.environ['COLUMNS']))
pandas.set_option('display.max_colwidth', 80)
pandas.set_option('display.max_columns', 100)


class AppOutput(object):
    """The container class for all trace related data.

    This class holds the relevant objects for parsing and indexing all
    data that is output from GEOPM.  This object can be created with a
    a trace glob string that will be used
    to search ``dir_name`` for the relevant files.  If files are found
    their data will be parsed into objects for easy data access.
    Additionally a :py:class:`pandas.DataFrame` is constructed containing all of the
    trace data.  These ``DataFrame``\ s are indexed based on the version of
    GEOPM found in the files, the profile name, agent name, and the number
    of times that particular configuration has been seen by the parser
    (i.e. experiment iteration).

    Attributes:
        trace_glob: The string pattern to use to search for trace files.
        dir_name: The directory path to use when searching for files.
        verbose: A ``bool`` to control whether verbose output is printed to stdout.

    """

    def __init__(self, traces=None, dir_name='.', verbose=False, do_cache=True):
        self._traces = {}
        self._traces_df = pandas.DataFrame()
        self._all_paths = []
        self._index_tracker = IndexTracker()
        self._node_names = None
        self._region_names = None

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
                try:
                    h5_id = hashlib.shake_256(paths_str.encode()).hexdigest(14)
                except AttributeError:
                    h5_id = hash(paths_str)
                trace_h5_name = 'trace_{}.h5'.format(h5_id)
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

                do_load_raw = True
                try:
                    self._traces_df = read_hdf(trace_h5_name, 'trace')
                    do_load_raw = False
                    if verbose:
                        sys.stdout.write(f'Loaded traces from {trace_h5_name}.\n')
                except ImportWarning as err:
                    sys.stderr.write(f'Warning: <geopm> geopmpy.io: {err}.\n')
                except IOError as err:
                    sys.stderr.write(f'Warning: <geopm> geopmpy.io: Trace HDF5 file not detected or older than traces {err}.\n')
                if do_load_raw:
                    self.parse_traces(trace_paths, verbose)
                    # Cache traces dataframe
                    try:
                        if verbose:
                            sys.stdout.write('Generating HDF5 files... ')
                        self._traces_df.to_hdf(trace_h5_name, 'trace')
                    except ImportError as error:
                        sys.stderr.write(f'Warning: <geopm> geopmpy.io: Unable to write HDF5 file: {error}\n')

                    if verbose:
                        sys.stdout.write('Done.\n')
                        sys.stdout.flush()
            else:
                self.parse_traces(trace_paths, verbose)

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

    def add_trace_df(self, tt, traces_df_list):
        """Adds a trace ``DataFrame`` to the tracking list.

        The report tracking list is used to create the combined
        ``DataFrame`` once all reports are parsed.

        Args:
            tt: The ``Trace`` object used to extract the Trace ``DataFrame``.
                This ``DataFrame`` will be indexed and added to the
                tracking list.

        """
        tdf = tt.get_df()  # TODO: this needs numeric cols optimization
        tdf = tdf.set_index(self._index_tracker.get_multiindex(tt))
        traces_df_list.append(tdf)

    def get_trace_data(self, node_name=None):
        idx = pandas.IndexSlice
        df = self._traces_df
        if node_name is not None:
            df = df.loc[idx[:, :, :, :, node_name, :, :], ]
        return df

    def get_trace_df(self):
        """Getter for the combined ``DataFrame`` of all trace files parsed.

        This ``DataFrame`` contains all data parsed, and has a complex
        ``MultiIndex`` for accessing the unique data from each individual
        trace.  For more information on this index, see the
        ``IndexTracker`` docstring.

        Returns:
            pandas.DataFrame: Contains all parsed data.

        """
        return self._traces_df


class IndexTracker(object):
    """Tracks and uniquely identifies experiment configurations for :py:class:`pandas.DataFrame` indexing.

    This object's purpose is to examine parsed data for reports or
    traces and determine if a particular experiment configuration has
    already been tracked.  A user may run the same configuration
    repeatedly in order to prove that results are repeatable and are
    not outliers.  Since the same configuration is used many times, it
    must be tracked and counted to ensure that the unique data for
    each run can be extracted later.

    The parsed data is used to extract the following fields to build
    the tracking index tuple:

    ``(<GEOPM_VERSION>, <PROFILE_NAME>, <AGENT_NAME>, <NODE_NAME>)``

    If the tuple is not contained in the ``_run_outputs`` dict, it is
    inserted with a value of 1.  The value is incremented if the tuple
    is currently in the ``_run_outputs`` dict.  This value is used to
    uniquely identify a particular set of parsed data when the
    ``MultiIndex`` is created.

    """
    def __init__(self):
        self._run_outputs = {}

    def _check_increment(self, run_output):
        """Extracts the index tuple from the parsed data and tracks it.

        Checks to see if the current ``run_output`` has been seen before.
        If so, the count is incremented.  Otherwise it is stored as 1.

        Args:
            run_output: The ``Trace`` object to be tracked.
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
           uniquely-identifying ``MultiIndex`` for this data.

        Takes a ``run_output`` as input, and returns the unique tuple to
        identify this ``run_output`` in the ``DataFrame``.  Note that this
        method appends the current experiment iteration to the end of
        the returned tuple.  E.g.:

        >>> self._index_tracker.get_base_index(rr)
        ('0.1.1+dev365gfcda929', 'geopm_test_integration', 170,
        'static_policy', 'power_balancing', 'mr-fusion2', 1)

        Args:
            run_output: The ``Trace`` object to produce an index tuple for.

        Returns:
            Tuple: This will contain all of the index fields needed to uniquely identify this data (including the
                count of how many times this experiment has been seen.

        """
        key = (run_output.get_version(), run_output.get_start_time(),
               os.path.basename(run_output.get_profile_name()),
               run_output.get_agent(), run_output.get_node_name())

        return key + (self._run_outputs[key], )

    def get_multiindex(self, run_output):
        """Returns a ``MultiIndex`` from this ``run_output``.  Used in :py:class:`pandas.DataFrame` construction.

        This will add the current ``run_output`` to the list of tracked
        data, and return a unique muiltiindex tuple to identify this
        data in a ``DataFrame``.

        For ``Trace`` objects, the integer index of the ``DataFrame`` is
        appended to the tuple.

        Args:
            run_output: The ``Trace`` object to produce an index
                        tuple for.

        Returns:
            pandas.MultiIndex: The unique index to identify this data object.

        """
        self._check_increment(run_output)

        itl = []
        index_names = ['version', 'start_time', 'name', 'agent', 'node_name', 'iteration']

        # Trace file index
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


class Trace(object):
    """Creates a :py:class:`pandas.DataFrame` comprised of the trace file data.

    This object will parse both the header and the CSV data in a trace
    file.  The header identifies the uniquely-identifying configuration
    for this file which is used for later indexing purposes.

    Even though ``__getattr__()`` and ``__getitem__()`` allow this object to
    effectively be treated like a ``DataFrame``, you must use ``get_df()`` if
    you're building a list of ``DataFrame``\ s to pass to ``pandas.concat()``.
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
                       'energy_package': 'CPU_ENERGY',
                       'energy_dram': 'DRAM_ENERGY',
                       'power_package': 'CPU_POWER',
                       'power_dram': 'DRAM_POWER',
                       'frequency': 'FREQUENCY',
                       'cycles_thread': 'CPU_CYCLES_THREAD',
                       'cycles_reference': 'CPU_CYCLES_REFERENCE',
                       'temperature_core': 'CPU_CORE_TEMPERATURE'}

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

        # Need to determine how many lines are in the header
        # explicitly.  We cannot use '#' as a comment character since
        # it occurs in raw MSR signal names.
        skiprows = 0
        with open(trace_path) as fid:
            for ll in fid:
                if ll.startswith('#'):
                    skiprows += 1
                else:
                    break
        column_headers = pandas.read_csv(trace_path, sep='|', skiprows=skiprows, nrows=0, encoding='utf-8').columns.tolist()
        original_headers = copy.deepcopy(column_headers)

        column_headers = [old_headers.get(ii, ii) for ii in column_headers]

        if column_headers != original_headers:
            sys.stderr.write('Warning: <geopm> geopmpy.io: Old trace file format detected. Old column headers will be forced ' \
                             'to UPPERCASE.\n')

        # region_hash and region_hint must be a string for pretty printing pandas DataFrames
        # You can force them to int64 by setting up a converter function then passing the hex string through it
        # with the read_csv call, but the number will be displayed as an integer from then on.  You'd have to convert
        # it back to a hex string to compare it with the data in the reports.
        self._df = pandas.read_csv(trace_path, sep='|', skiprows=skiprows, header=0, names=column_headers, encoding='utf-8',
                                   dtype={'REGION_HASH': 'unicode', 'REGION_HINT': 'unicode'})
        self._df.columns = list(map(str.strip, self._df[:0]))  # Strip whitespace from column names
        try:
            self._df['REGION_HASH'] = self._df['REGION_HASH'].astype('unicode').map(str.strip)  # Strip whitespace from region hashes
            self._df['REGION_HINT'] = self._df['REGION_HINT'].astype('unicode').map(str.strip)  # Strip whitespace from region hints
        except KeyError: # Hash and hint are not present in profile traces
            pass

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
            dd = yaml.safe_load(yaml_fd)
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
        """Diff the ``DataFrame``.

        Since the counters in the trace files are monotonically
        increasing, a diff must be performed to extract the useful
        data.

        Args:
            trace_df: The ``MultiIndex``\ ed :py:class:`pandas.DataFrame` created by the
                      ``AppOutput`` class.
            column_regex: A string representing the regex search
                          pattern for the column names to diff.
            epoch: A flag to set whether or not to focus solely on
                   epoch regions.

        Returns:

            pandas.DataFrame: With the diffed columns specified by ``'column_regex'``, and an ``'elapsed_time'`` column.

        TODO:
            * Should I drop everything before the first epoch if
              ``'epoch'`` is false?

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
        # If the epoch_count column is included, this will also drop rows occurring mid-epoch.
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
        then extracts the ``DataFrame`` for the iteration that is closest
        to the median.  For input ``DataFrame``\ s with a single iteration,
        the single iteration is returned.

        Args:
            trace_df: The ``MultiIndex``\ ed :py:class:`pandas.DataFrame` created by the
                      ``AppOutput`` class.
            column_regex: A string representing the regex search
                          pattern for the column names to diff.
            config: The ``TraceConfig`` object being used presently.

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
                       ``[0,1]``.  For example, specifying a value of 0.25
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


class RawReport(object):
    def __init__(self, path):
        # Fix issue with python yaml module where it is confused
        # about floating point numbers of the form "1e+10" where
        # the decimal point is missing.
        # See PR: https://github.com/yaml/pyyaml/pull/174
        # for upstream fix to pyyaml
        yaml.SafeLoader.add_implicit_resolver(
            u'tag:yaml.org,2002:float',
            re.compile(r'''^(?:[-+]?(?:[0-9][0-9_]*)\.[0-9_]*(?:[eE][-+]?[0-9]+)?
                           |[-+]?(?:[0-9][0-9_]*)(?:[eE][-+]?[0-9]+)
                           |\.[0-9_]+(?:[eE][-+]?[0-9]+)?
                           |[-+]?[0-9][0-9_]*(?::[0-5]?[0-9])+\.[0-9_]*
                           |[-+]?\.(?:inf|Inf|INF)
                           |\.(?:nan|NaN|NAN))$''', re.X),
            list(u'-+0123456789.'))
        with open(path) as fid:
            self._raw_dict = yaml.safe_load(fid)

    def raw_report(self):
        return copy.deepcopy(self._raw_dict)

    def dump_json(self, path):
        jdata = json.dumps(self._raw_dict)
        with open(path, 'w') as fid:
            fid.write(jdata)

    def meta_data(self):
        result = dict()
        all_keys = ['GEOPM Version',
                    'Start Time',
                    'Profile',
                    'Agent',
                    'Policy']
        for kk in all_keys:
            result[kk] = self._raw_dict[kk]
        return result

    def figure_of_merit(self):
        result = None
        try:
            result = copy.deepcopy(self._raw_dict['Figure of Merit'])
        except KeyError:
            pass
        return result

    def total_runtime(self):
        result = None
        try:
            result = copy.deepcopy(self._raw_dict['Total Runtime'])
        except KeyError:
            pass
        return result

    def host_names(self):
        return list(self._raw_dict['Hosts'].keys())

    def region_names(self, host_name):
        result = []
        try:
            result = [rr['region'] for rr in self._raw_dict['Hosts'][host_name]['Regions']]
        except KeyError:
            pass
        return result

    def raw_region(self, host_name, region_name):
        result = None
        try:
            for rr in self._raw_dict['Hosts'][host_name]['Regions']:
                if rr['region'] == region_name:
                    result = copy.deepcopy(rr)
        except KeyError:
            raise RuntimeError('report has no regions for the specified host')
        if not result:
            raise RuntimeError('region name: {} not found'.format(region_name))
        return result

    def raw_unmarked(self, host_name):
        host_data = self._raw_dict["Hosts"][host_name]
        key = 'Unmarked Totals'
        return copy.deepcopy(host_data[key])

    def raw_epoch(self, host_name):
        host_data = self._raw_dict["Hosts"][host_name]
        key = 'Epoch Totals'
        return copy.deepcopy(host_data[key])

    def raw_totals(self, host_name):
        host_data = self._raw_dict["Hosts"][host_name]
        key = 'Application Totals'
        return copy.deepcopy(host_data[key])

    def agent_host_additions(self, host_name):
        # other keys that are not region, epoch, or app
        # total i.e. from Agent::report_host()
        host_data = self._raw_dict[host_name]
        result = {}
        for key, val in host_data.items():
            if key not in ['Epoch Totals', 'Application Totals'] and not key.startswith('Region '):
                result[key] = copy.deepcopy(val)
        return result

    def get_field(self, raw_data, key, units=''):
        matches = [(len(kk), kk) for kk in raw_data if key in kk and units in kk]
        if len(matches) == 0:
            raise KeyError('<geopm> geopmpy.io: Field not found: {}'.format(key))
        match = sorted(matches)[0][1]
        return copy.deepcopy(raw_data[match])


class RawReportCollection(object):
    '''
    Used to group together a collection of related :py:class:`geopmpy.io.RawReport`\ s.
    '''

    def __init__(self, report_paths, dir_name='.', dir_cache=None, verbose=True, do_cache=True):
        self._reports_df = pandas.DataFrame()
        self._app_reports_df = pandas.DataFrame()
        self._epoch_reports_df = pandas.DataFrame()
        self._unmarked_reports_df = pandas.DataFrame()
        self._meta_data = None
        self.load_reports(report_paths, dir_name, dir_cache, verbose, do_cache)

    @staticmethod
    def make_h5_name(paths, outdir):
        paths_str = str([os.path.realpath(rr) for rr in paths])
        h5_id = hashlib.sha256(paths_str.encode()).hexdigest()[:14]
        report_h5_name = os.path.join(outdir, 'cache_{}.h5'.format(h5_id))
        return report_h5_name

    @staticmethod
    def fixup_metadata(metadata, df):
        # This block works around having mixed datatypes in the Profile column by
        # forcing the column into the NumPy S type.
        for key, val in metadata.items():
            if type(val) is not dict: # Policy is a dict and should be excluded
                df[key] = df[key].astype('S')
        return df

    def load_reports(self, reports, dir_name, dir_cache, verbose, do_cache):
        '''
        TODO:
            * copied from ``AppOutput``.  refactor to shared function.
            * removed concept of tracked files to be deleted
            * added separate epoch dataframe
        '''
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

        if do_cache:
            if dir_cache is None:
                dir_cache = dir_name
            self._report_h5_name = RawReportCollection.make_h5_name(report_paths, dir_cache)
            # check if cache is older than reports
            if os.path.exists(self._report_h5_name):
                cache_mod_time = os.path.getmtime(self._report_h5_name)
                regen_cache = False
                for report_file in report_paths:
                    mod_time = os.path.getmtime(report_file)
                    if mod_time > cache_mod_time:
                        regen_cache = True
                if regen_cache:
                    os.remove(self._report_h5_name)

            try:
                if verbose:
                    sys.stdout.write('Attempting to read {}...\n'.format(self._report_h5_name))
                # load dataframes from cache
                try:
                    self._reports_df = read_hdf(self._report_h5_name, 'report')
                except KeyError:
                    pass # No regions in cached report
                self._app_reports_df = read_hdf(self._report_h5_name, 'app_report')
                try:
                    self._unmarked_reports_df = read_hdf(self._report_h5_name, 'unmarked_report')
                except KeyError:
                    pass
                try:
                    self._epoch_reports_df = read_hdf(self._report_h5_name, 'epoch_report')
                except KeyError:
                    pass
                if verbose:
                    sys.stdout.write('Loaded report data from {}.\n'.format(self._report_h5_name))
            except (IOError, ImportWarning) as err:
                if type(err) is IOError:
                    sys.stderr.write('Warning: <geopm> geopmpy.io: Report HDF5 file not detected or older than reports.\n')
                else:
                    sys.stderr.write(f'Warning: <geopm> geopmpy.io: {err}.\n')

                self.parse_reports(report_paths, verbose)

                # Cache report dataframe
                cache_created = False
                while not cache_created:
                    try:
                        if verbose:
                            sys.stdout.write('Generating HDF5 files... ')
                        self._app_reports_df.to_hdf(self._report_h5_name, 'app_report', format='table')
                        if len(self._reports_df) > 0:
                            self._reports_df.to_hdf(self._report_h5_name, 'report', format='table', append=True)
                        if len(self._unmarked_reports_df) > 0:
                            self._unmarked_reports_df.to_hdf(self._report_h5_name, 'unmarked_report', format='table', append=True)
                        if len(self._epoch_reports_df) > 0:
                            self._epoch_reports_df.to_hdf(self._report_h5_name, 'epoch_report', format='table', append=True)
                        cache_created = True
                    except TypeError as error:
                        fm = RawReportCollection.fixup_metadata
                        if verbose:
                            sys.stdout.write('Applying workaround for strings in HDF5 files... ')
                        self._reports_df = fm(self._meta_data, self._reports_df)
                        self._app_reports_df = fm(self._meta_data, self._app_reports_df)
                        self._unmarked_reports_df = fm(self._meta_data, self._unmarked_reports_df)
                        self._epoch_reports_df = fm(self._meta_data, self._epoch_reports_df)
                    except ImportError as error:
                        sys.stderr.write('Warning: <geopm> geopmpy.io: Unable to write HDF5 file: {}\n'.format(str(error)))
                        break

                if verbose:
                    sys.stdout.write('Done.\n')
                    sys.stdout.flush()

        else:
            self.parse_reports(report_paths, verbose)

    def parse_reports(self, report_paths, verbose):
        # Note: overlapping key names can break this
        # Insert repeated data for non-leaf levels
        # TODO: iteration - for now, distinguishable from start time

        def _init_tables():
            self._columns_order = {}
            self._columns_set = {}
            for name in ['region', 'unmarked', 'epoch', 'app']:
                self._columns_order[name] = []
                self._columns_set[name] = set()

        def _add_column(table_name, col_name):
            '''
            Used to add columns to the data frame in the order they appear in the report.
            '''
            if table_name not in ['region', 'unmarked', 'epoch', 'app', 'all']:
                raise RuntimeError('Invalid table name')
            if table_name == 'all':
                _add_column('region', col_name)
                _add_column('unmarked', col_name)
                _add_column('epoch', col_name)
                _add_column('app', col_name)
            elif col_name not in self._columns_set[table_name]:
                self._columns_set[table_name].add(col_name)
                self._columns_order[table_name].append(col_name)

        def _try_float(val):
            '''
            Attempt to convert values we assume are floats
            '''
            rv = val
            try:
                rv = float(val)
            except ValueError:
                pass
            return rv

        _init_tables()

        region_df_list = []
        unmarked_df_list = []
        epoch_df_list = []
        app_df_list = []

        for report in report_paths:
            if verbose:
                sys.stdout.write("Loading data from {}.\n".format(report))
            rr = RawReport(report)
            self._meta_data = rr.meta_data()
            header = {}

            # report header
            for top_key, top_val in self._meta_data.items():
                # allow one level of dict nesting in header for policy
                if type(top_val) is dict:
                    for in_key, in_val in top_val.items():
                        _add_column('all', in_key)
                        header[in_key] = _try_float(in_val)
                else:
                    _add_column('all', top_key)
                    header[top_key] = str(top_val)

            _add_column('all', 'host')
            figure_of_merit = rr.figure_of_merit()
            if figure_of_merit is not None:
                _add_column('app', 'FOM')
            total_runtime = rr.total_runtime()
            if total_runtime is not None:
                _add_column('app', 'total_runtime')
            host_names = rr.host_names()
            for host in host_names:
                # data about host to be repeated over all rows
                per_host_data = {'host': host}

                # TODO: other host data may also contain dict
                # TODO: leave out for now
                #other_host_data = rr.agent_host_additions(host)

                _add_column('region', 'region')
                region_names = rr.region_names(host)
                for region in region_names:
                    row = copy.deepcopy(header)
                    row.update(per_host_data)
                    #row['region'] = row.pop('name')
                    # TODO: region hash
                    region_data = rr.raw_region(host, region)
                    for key, val in region_data.items():
                        region_data[key] = _try_float(val)
                    row.update(region_data)
                    for cc in rr.raw_region(host, region).keys():
                        _add_column('region', cc)
                    region_df_list.append(pandas.DataFrame(row, index=[0]))

                unmarked_row = copy.deepcopy(header)
                unmarked_row.update(per_host_data)
                try:
                    unmarked_data = rr.raw_unmarked(host)
                    for key, val in unmarked_data.items():
                        unmarked_data[key] = _try_float(val)
                    for cc in unmarked_data.keys():
                        _add_column('unmarked', cc)
                    unmarked_row.update(unmarked_data)
                    unmarked_df_list.append(pandas.DataFrame(unmarked_row, index=[0]))
                except KeyError:
                    pass

                epoch_row = copy.deepcopy(header)
                epoch_row.update(per_host_data)
                try:
                    epoch_data = rr.raw_epoch(host)
                    for key, val in epoch_data.items():
                        epoch_data[key] = _try_float(val)
                    for cc in epoch_data.keys():
                        _add_column('epoch', cc)
                    epoch_row.update(epoch_data)
                    epoch_df_list.append(pandas.DataFrame(epoch_row, index=[0]))
                except KeyError:
                    pass

                app_row = copy.deepcopy(header)
                if figure_of_merit is not None:
                    app_row['FOM'] = figure_of_merit
                if total_runtime is not None:
                    app_row['total_runtime'] = total_runtime
                app_row.update(per_host_data)
                app_data = rr.raw_totals(host)
                for key, val in app_data.items():
                    app_data[key] = _try_float(val)
                for cc in app_data.keys():
                    _add_column('app', cc)
                app_row.update(app_data)
                app_df_list.append(pandas.DataFrame(app_row, index=[0]))
        # reorder the columns to order of first appearance
        try:
            df = pandas.concat(region_df_list, ignore_index=True)
            df = df.reindex(columns=self._columns_order['region'])
            self._reports_df = df
        except ValueError: # If there were no regions
            pass
        try:
            unmarked_df = pandas.concat(unmarked_df_list, ignore_index=True)
            unmarked_df = unmarked_df.reindex(columns=self._columns_order['unmarked'])
            self._unmarked_reports_df = unmarked_df
        except ValueError: # If there were no unmarked regions (non-MPI app)
            pass
        try:
            epoch_df = pandas.concat(epoch_df_list, ignore_index=True)
            epoch_df = epoch_df.reindex(columns=self._columns_order['epoch'])
            self._epoch_reports_df = epoch_df
        except (KeyError, ValueError):
            pass
        app_df = pandas.concat(app_df_list, ignore_index=True)
        app_df = app_df.reindex(columns=self._columns_order['app'])
        self._app_reports_df = app_df

    def remove_cache(self):
        try:
            os.unlink(self._report_h5_name)
        except OSError:
            pass

    # TODO: rename
    def get_df(self):
        return self._reports_df

    def get_df_filtered(self, columns):
        return self.reports_df.drop(columns=columns)

    def get_epoch_df(self):
        return self._epoch_reports_df

    def get_app_df(self):
        return self._app_reports_df

    def get_unmarked_df(self):
        return self._unmarked_reports_df
