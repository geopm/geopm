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

from contextlib import contextmanager
import os
import pipes
import re
import shutil
import sys
import unittest
import subprocess
from io import StringIO

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
from test_integration import geopm_test_launcher

def skip_unless_platform_bdx():
    fam, mod = geopm_test_launcher.get_platform()
    if fam != 6 or mod not in (45, 47, 79):
        return unittest.skip("Performance test is tuned for BDX server, The family {}, model {} is not supported.".format(fam, mod))
    return lambda func: func


def skip_unless_config_enable(feature):
    path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.realpath(__file__))),
           'config.log')
    with open(path) as fid:
        for line in fid.readlines():
            if line.startswith("enable_{}='0'".format(feature)):
                return unittest.skip("Feature: {feature} is not enabled, configure with --enable-{feature} to run this test.".format(feature=feature))
            elif line.startswith("enable_{}='1'".format(feature)):
                break
    return lambda func: func


def skip_unless_optimized():
    path = os.path.join(
           os.path.dirname(
            os.path.dirname(
             os.path.realpath(__file__))),
           'config.log')
    with open(path) as fid:
        for line in fid.readlines():
            if line.startswith("enable_debug='1'"):
                return unittest.skip("This performance test cannot be run when GEOPM is configured with --enable-debug")
    return lambda func: func


def skip_unless_batch():
    batch_env_vars = ['SLURM_NODELIST', 'COBALT_JOBID']
    if not any(opt in batch_env_vars for opt in os.environ):
        return unittest.skip('Requires batch session.')
    return lambda func: func


def skip_unless_run_long_tests():
    if 'GEOPM_RUN_LONG_TESTS' not in os.environ:
        return unittest.skip("Define GEOPM_RUN_LONG_TESTS in your environment to run this test.")
    return lambda func: func


def skip_unless_cpufreq():
    try:
        test_exec = "dummy -- stat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq \
                     && stat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"
        dev_null = open('/dev/null', 'w')
        geopm_test_launcher.allocation_node_test(test_exec, dev_null, dev_null)
        dev_null.close()
    except subprocess.CalledProcessError:
        return unittest.skip("Could not determine min and max frequency, enable cpufreq driver to run this test.")
    return lambda func: func

def skip_or_ensure_writable_file(path):
    """Skip the test unless the given file can be created or modified on compute nodes.
    """
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

def skip_if_is_slurm_and_missing_geopm_spank():
    """Skip the test if using SLURM without the GEOPM SPANK plugin.
    """
    if geopm_test_launcher.detect_launcher() == "srun":
        try:
            with open('/dev/null', 'w') as dev_null:
                stderr = StringIO()
                geopm_test_launcher.allocation_node_test('dummy -- echo', dev_null, stderr)
                stderr.seek(0)
                # Search for the slurm_info() call when the plugin is initialized so we can
                # avoid trying to parse all of the plugstack-related config options
                if not any('Loaded geopm plugin' in line for line in stderr):
                    return unittest.skip("Using SLURM without the GEOPM SPANK plugin")
        except subprocess.CalledProcessError:
            return unittest.skip("Unable to check if the GEOPM SPANK plugin is loaded")
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
