#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


from contextlib import contextmanager
import os
import pipes
import re
import shutil
import sys
import unittest
import subprocess
from io import StringIO
import argparse

from docutils.nodes import literal_block
from docutils.core import publish_doctree

from integration.test import geopm_test_launcher


# the global singleton
class Util:
    _instance = None

    def __init__(self):
        self._do_parse_once = False
        self._skip_launch = False

    @classmethod
    def instance(cls):
        if not cls._instance:
            cls._instance = Util()
            cls._instance.parse_command_line_args()
        return cls._instance

    def parse_command_line_args(self):
        ''' Parses command line to remove geopm integration-specific options.
            New options added here should not conflict with the set
            provided by unittest.main().
            @todo: perhaps a geopm- prefix for options?
        '''
        if self._do_parse_once:
            return

        self._do_parse_once = True
        parser = argparse.ArgumentParser()
        parser.add_argument('--skip-launch', dest='skip_launch',
                            action='store_true', default=False,
                            help='reuse existing data files; do not launch any jobs')
        # workaround so that this help plus base pyunit help text will be printed
        if '-h' in sys.argv or '--help' in sys.argv:
            # TODO: use custom help text; this is confusing
            sys.stdout.write('GEOPM test-specific help:\n')
            sys.stdout.write('-------------------------\n')
            parser.print_help()
            sys.stdout.write('\nCommon pyunit help:\n')
            sys.stdout.write(  '-------------------\n')
            return

        args, remaining = parser.parse_known_args()
        sys.argv = [sys.argv[0]] + remaining
        self._skip_launch = args.skip_launch

    def skip_launch(self):
        ''' Skip parts of test requiring a job launched on compute nodes.
            Ideally the test should only analyze existing report/trace files.
        '''
        return self._skip_launch


# will be created the first time this file is imported
g_util = Util.instance()


def do_launch():
    return not g_util.skip_launch()

def skip_unless_do_launch():
    if not do_launch():
        return unittest.skip("Most tests in this suite require launch; do not set --skip-launch.")
    return lambda func: func

def skip_unless_workload_exists(path):
    path = os.path.join(
            os.path.dirname(
             os.path.dirname(os.path.realpath(__file__))),
            path)
    if not os.path.exists(path):
        return unittest.skip("Could not find workload executable {}, skipping test.".format(path))

    return lambda func: func

def skip_unless_geopmread(signal_and_domain):
    #try/catch geopmread of signal
    try:
        read = geopm_test_launcher.geopmread(signal_and_domain)
    except subprocess.CalledProcessError:
        return unittest.skip("geopmread {} not supported, skipping test.".format(signal_and_domain))
    return lambda func: func

def skip_unless_levelzero():
    return skip_unless_service_config_enable('levelzero');

def skip_unless_nvml():
    return skip_unless_service_config_enable('nvml');

def skip_unless_gpu():
    #try/catch read signal for gpu power
    try:
        power = geopm_test_launcher.geopmread("GPU_ENERGY gpu 0")
    except subprocess.CalledProcessError:
        return unittest.skip("Read of GPU_ENERGY not supported, skipping test.")
    return lambda func: func

def skip_unless_workload_exists(path):
    path = os.path.join(
            os.path.dirname(
             os.path.dirname(os.path.realpath(__file__))),
            path)
    if not os.path.exists(path):
        return unittest.skip("Could not find workload executable {}, skipping test.".format(path))

    return lambda func: func

def skip_unless_platform_bdx():
    fam, mod = geopm_test_launcher.get_platform()
    if fam != 6 or mod not in (45, 47, 79):
        return unittest.skip("Performance test is tuned for BDX server, The family {}, model {} is not supported.".format(fam, mod))
    return lambda func: func

def get_config_log():
    path = os.path.join(os.environ['GEOPM_SOURCE'], 'libgeopm', 'config.log')
    return path

def get_service_config_log():
    path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.dirname(
              os.path.realpath(__file__)))),
           'service/config.log')
    if not os.path.isfile(path):
        path = os.path.join(
               os.path.dirname(path),
                'integration/build/config.log')
    return path

def get_config_value(key):
    """Get the value of an option from the build configuration, returning None
    if no such key is present.
    """
    path = get_config_log()
    with open(path) as config_file:
        for line in config_file:
            line_start = "{}='".format(key)
            line_end = "'\n"
            if line.startswith(line_start) and line.endswith(line_end):
                return line[len(line_start):-len(line_end)]
    return None

def get_service_config_value(key):
    """Get the value of an option from the service build configuration,
    returning None if no such key is present.
    """
    path = get_service_config_log()
    with open(path) as config_file:
        for line in config_file:
            line_start = "{}='".format(key)
            line_end = "'\n"
            if line.startswith(line_start) and line.endswith(line_end):
                return line[len(line_start):-len(line_end)]
    return None

def get_exec_path(bin_name):
    """Find test binary

    """
    path_list = []
    int_test_dir = os.path.dirname(os.path.realpath(__file__))
    path_list.append(os.path.join(int_test_dir, '.libs'))
    int_dir = os.path.dirname(int_test_dir)
    # Look for out of place build from using apps/build_func.sh
    path_list.append(os.path.join(int_dir, 'build/integration/test/.libs'))
    bin_list = [os.path.join(pp, bin_name) for pp in path_list]
    for bb in bin_list:
        if os.path.isfile(bb):
            return bb

    sep = '\n    '
    msg = 'Could not find application binary, tried {}{}'.format(sep, sep.join(bin_list))
    raise RuntimeError(msg)

def skip_unless_config_enable(feature):
    if get_config_value('enable_{}'.format(feature)) == '1':
        return lambda func: func
    else:
        return unittest.skip("Feature: {feature} is not enabled, configure with --enable-{feature} to run this test.".format(feature=feature))

def skip_unless_service_config_enable(feature):
    if get_service_config_value('enable_{}'.format(feature)) == '1':
        return lambda func: func
    else:
        return unittest.skip("Feature: {feature} is not enabled in service, configure with --enable-{feature} to run this test.".format(feature=feature))

def skip_unless_optimized():
    path = get_config_log()
    with open(path) as fid:
        for line in fid.readlines():
            if line.startswith("enable_debug='1'"):
                return unittest.skip("This performance test cannot be run when GEOPM is configured with --enable-debug")
    return lambda func: func


def skip_unless_batch():
    batch_env_vars = ['SLURM_NODELIST', 'COBALT_JOBID', 'PBS_JOBID']
    if not g_util.skip_launch() and not any(opt in batch_env_vars for opt in os.environ):
        return unittest.skip('Requires batch session.')
    return lambda func: func


def skip_unless_cpufreq():
    if not g_util.skip_launch():
        try:
            test_exec = "dummy -- stat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq"
            dev_null = open('/dev/null', 'w')
            geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
            dev_null.close()
        except subprocess.CalledProcessError:
            return unittest.skip("Could not determine min frequency, enable cpufreq driver to run this test.")
    return lambda func: func


def skip_unless_stressng():
    if not g_util.skip_launch():
        try:
            test_exec = "dummy -- stress-ng -h"
            dev_null = open('/dev/null', 'w')
            geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
            dev_null.close()
        except subprocess.CalledProcessError:
            return unittest.skip("Missing stress-ng.  Please install in the compute image.")
    return lambda func: func


def skip_or_ensure_writable_file(path):
    """Skip the test unless the given file can be created or modified on compute nodes.
    """
    if g_util.skip_launch():
        return unittest.skip("This test requires launch; do not use --skip-launch")

    stdout = StringIO()
    created_directories = list()
    try:
        dirname = os.path.dirname(path)
        with open('/dev/null', 'w') as dev_null:
            # Ensure the parent directory exists
            geopm_test_launcher.allocation_node_test('dummy -- mkdir -vp {}'.format(dirname), stdout, dev_null)
            stdout.seek(0)
            pattern = re.compile(r"mkdir: created directory '([^']+)'")
            for line in stdout:
                match = pattern.search(line)
                if match:
                    created_directories.append(match.group(1))

            run_script_on_compute_nodes("test '(' '!' -e {path} -a -w {dirname} ')' -o -w {path}".format(
                path=pipes.quote(path), dirname=pipes.quote(dirname)), sys.stdout, sys.stdout)
    except subprocess.CalledProcessError as e:
        return unittest.skip("Cannot write to path: {}".format(path))

    def cleanup_decorator(func):
        def cleanup_wrapper(*args, **kwargs):
            try:
                func(*args, **kwargs)
            finally:
                for directory in reversed(created_directories):
                    shutil.rmtree(directory)
        return cleanup_wrapper

    return cleanup_decorator


def skip_unless_library_in_ldconfig(library):
    """Skip the test if the given library is not in ldconfig on the compute nodes.
    """
    if not g_util.skip_launch():
        try:
            with open('/dev/null', 'w') as dev_null:
                stdout = StringIO()
                geopm_test_launcher.allocation_node_test('dummy -- ldconfig --print-cache', stdout, dev_null)
                stdout.seek(0)
                if not any(line.startswith('\t{}'.format(library)) for line in stdout):
                    return unittest.skip("Library '{}' not in ldconfig".format(library))
        except subprocess.CalledProcessError:
            return unittest.skip("Unable to run ldconfig")
    return lambda func: func


def skip_unless_msr_access(msg=None):
    """Skip the test if the user does not have direct access to msr-safe
    """
    if not g_util.skip_launch():
        try_path = '/dev/cpu/msr_batch'
        try:
            test_exec = 'dummy -- test -w {}'.format(try_path)
            with open('/dev/null', 'w') as dev_null:
                geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
        except subprocess.CalledProcessError:
            if msg is None:
                msg = 'No msr-safe access.  Cannot write to ' + try_path
            return unittest.skip(msg)
    return lambda func: func


def skip_unless_no_service_or_root():
    if not g_util.skip_launch():
        try:
            test_exec = "dummy -- systemctl status geopm"
            with open('/dev/null', 'w') as dev_null:
                geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
        except subprocess.CalledProcessError:
            return lambda func: func

        if os.getuid() == 0:
                return lambda func: func
        else:
            return unittest.skip("The service is currently installed and running AND you are not root.")

    return lambda func: func


def run_script_on_compute_nodes(script, stdout, stderr, interpreter='sh'):
    """Run an inline script on compute nodes.
    """
    geopm_test_launcher.allocation_node_test('dummy -- {}'.format(
        '{} -c {}'.format(interpreter, pipes.quote(script))), stdout, stderr)


@contextmanager
def temporarily_remove_compute_node_file(path):
    """Context manager to remove a file from compute nodes if it exists, by renaming
    with a .backup suffix.  When exiting the context, the file is restored by moving
    the backup file to its original path.
    """
    with open('/dev/null', 'w') as dev_null:
        try:
            run_script_on_compute_nodes("test '!' -e {path} || mv -f {path} {path}.backup".format(path=path),
                                        dev_null, dev_null)
            yield path
        finally:
            run_script_on_compute_nodes("rm -f {path} && test '!' -e {path}.backup || mv {path}.backup {path}".format(path=pipes.quote(path)),
                                        dev_null, dev_null)


def remove_file_on_compute_nodes(file_path):
    """Remove a file from compute nodes.
    """
    with open('/dev/null', 'w') as dev_null:
        geopm_test_launcher.allocation_node_test('dummy -- rm {}'.format(file_path), dev_null, dev_null)


def assertNear(self, a, b, epsilon=0.05, msg=''):
    denom = a if a != 0 else 1
    if abs((a - b) / denom) >= epsilon:
        self.fail('The fractional difference between {a} and {b} is greater than {epsilon}.  {msg}'.format(a=a, b=b, epsilon=epsilon, msg=msg))


def assertNotNear(self, a, b, epsilon=0.05, msg=''):
    denom = a if a != 0 else 1
    if abs((a - b) / denom) < epsilon:
        self.fail('The fractional difference between {a} and {b} is less than {epsilon}.  {msg}'.format(a=a, b=b, epsilon=epsilon, msg=msg))


def get_scripts_from_readme(rst_readme_path):
    """Return a list of the sh-formatted scripts that are embedded in a .rst file.
    """
    script_bodies = list()
    with open(rst_readme_path) as f:
        # Syntax highlighting is enabled by default. If the syntax highlighter
        # (Pygments) is not installed, the tree-walk will skip over code blocks
        # and this function will return an empty list. Explicitly disable
        # highlighting so we can run this test regardless of whether Pygments
        # is installed.
        doctree = publish_doctree(f.read(), settings_overrides={'syntax_highlight': 'none'})
        for code_block in doctree.findall(literal_block):
            if 'sh' in code_block['classes']:
                script_bodies.append(code_block.astext())
    return script_bodies

def get_num_node():
    return int(os.environ.get("GEOPM_NUM_NODE", os.environ.get("SLURM_NNODES", 4)))
