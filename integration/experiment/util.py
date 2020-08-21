#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
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

# Helper functions useful for both tests and experiments

import sys
import os
import socket
import subprocess
import io
import yaml
import shlex

import geopmpy.launcher


def detect_launcher():
    """
    Heuristic to determine the resource manager used on the system.
    Returns name of resource manager or launcher, otherwise a
    LookupError is raised.
    """
    # Try the environment
    result = os.environ.get('GEOPM_LAUNCHER', None)
    if not result:
        # Check for known host names
        slurm_hosts = ['mr-fusion', 'mcfly']
        alps_hosts = ['theta']
        hostname = socket.gethostname()
        if any(hostname.startswith(word) for word in slurm_hosts):
            result = 'srun'
        elif any(hostname.startswith(word) for word in alps_hosts):
            result = 'aprun'
    if not result:
        try:
            exec_str = 'srun --version'
            subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, shell=True)
            result = 'srun'
        except subprocess.CalledProcessError:
            pass
    if not result:
        try:
            exec_str = 'aprun --version'
            subprocess.check_call(exec_str, stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE, shell=True)
            result = 'aprun'
        except subprocess.CalledProcessError:
            pass
    if not result:
        raise LookupError('Unable to determine resource manager')
    return result


def allocation_node_test(test_exec, stdout, stderr):
    argv = shlex.split(test_exec)
    launcher = detect_launcher()
    argv.insert(1, launcher)
    if launcher == 'aprun':
        argv.insert(2, '-q')  # Use quiet flag with aprun to suppress end of job info string
    argv.insert(2, '--geopm-ctl-disable')
    launcher = geopmpy.launcher.Factory().create(argv, num_rank=1, num_node=1,
                                                 job_name="geopm_allocation_test", quiet=True)
    launcher.run(stdout, stderr)


def geopmwrite(write_str):
    test_exec = "dummy -- geopmwrite " + write_str
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        sys.stderr.write(stderr.getvalue())
        raise err
    output = stdout.getvalue().splitlines()
    last_line = None
    if len(output) > 0:
        last_line = output[-1]
    return last_line


def geopmread(read_str):
    test_exec = "dummy -- geopmread " + read_str
    stdout = io.StringIO()
    stderr = io.StringIO()
    try:
        allocation_node_test(test_exec, stdout, stderr)
    except subprocess.CalledProcessError as err:
        sys.stderr.write(stderr.getvalue())
        raise err
    output = stdout.getvalue()
    last_line = output.splitlines()[-1]

    if last_line.startswith('0x'):
        result = int(last_line)
    else:
        try:
            result = float(last_line)
        except ValueError:
            result = yaml.safe_load(output).values()[0]
    return result


def launch_run(agent_conf, app_conf, run_id, output_dir, extra_cli_args, log_path=None,
               **launcher_factory_args):

    # TODO: why does launcher strip off first arg, rather than geopmlaunch main?
    argv = ['dummy', detect_launcher()]

    agent_name = 'monitor'
    if agent_conf is not None:
        if os.path.dirname(agent_conf.get_path()) != output_dir:
            raise RuntimeError("Agent policy is not located in output_dir")
        agent_name = agent_conf.get_agent()
        agent_conf.write()

    if agent_name != 'monitor':
        argv.append('--geopm-agent=' + agent_conf.get_agent())
        argv.append('--geopm-policy=' + agent_conf.get_path())

    app_name = app_conf.name()

    uid = '{}_{}_{}'.format(app_name.lower(), agent_name, run_id)
    report_path = os.path.join(output_dir, '{}.report'.format(uid))
    trace_path = os.path.join(output_dir, '{}.trace'.format(uid))
    profile_name = uid
    log_path = os.path.join(output_dir, '{}.log'.format(uid))

    # TODO: these are not passed to launcher create()
    # some are generic enough they could be, though
    extra_cli_args = ['--geopm-report', report_path,
                      '--geopm-trace', trace_path,
                      '--geopm-profile', profile_name,
    ]
    argv.extend(extra_cli_args)

    # extra geopm args needed by app
    argv.extend(app_conf.get_custom_geopm_args())

    argv.extend(['--'])

    bash_path = app_conf.make_bash(output_dir, uid)
    argv.extend([bash_path])

    launcher = geopmpy.launcher.Factory().create(argv, **launcher_factory_args)
    if log_path is None:
        launcher.run()
    else:
        with open(log_path, 'w') as outfile:
            # TODO: some apps print to stderr..... use stderr=outfile
            launcher.run(stdout=outfile)

    # Get app-reported figure of merit
    fom = app_conf.parse_fom(log_path)
    # Append to report????????????????
    with open(report_path, 'a') as report:
        report.write('\nFigure of Merit: {}'.format(fom))
