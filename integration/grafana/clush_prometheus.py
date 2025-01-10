#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import re
import datetime
import subprocess # nosec
import signal
from getpass import getuser
from argparse import ArgumentParser
from time import sleep
from tempfile import NamedTemporaryFile

def pbs_host_list(jobid):
    """Get list of hostnames from a PBS jobid

    """
    PBS_POLL_TIME = 15
    jobid = str(jobid)
    cmd = ['qstat', jobid]
    is_ready = False
    while not is_ready:
        run_ret = subprocess.run(cmd, stdout=subprocess.PIPE, check=True)
        last_line = run_ret.stdout.decode().split('\n')[-2]
        line_split = last_line.split()
        if len(line_split) >= 5 and line_split[4] == 'R':
            is_ready = True
        else:
            sleep(PBS_POLL_TIME)
    cmd = ['qstat', '-x', '-f', '-w', jobid]
    run_ret = subprocess.run(cmd, stdout=subprocess.PIPE, check=True)
    host_str = None
    for line in run_ret.stdout.decode().split('\n'):
        if 'exec_host' in line:
            line_split = line.split()
            if len(line_split) >= 3:
                host_str = line_split[2]
                break
    if host_str is None:
        raise RuntimeError('Failed to parse host list from qstat output')
    hosts =[xx[:xx.find('/')] for xx in host_str.split('+')]
    hosts = list(set(hosts))
    hosts.sort()
    return hosts

def popen_wrapper(cmd, cmd_name, log_path):
    cmd_name = cmd_name.upper()
    print(f'{cmd_name} COMMAND: {" ".join(cmd)}')
    print(f'{cmd_name} LOGFILE: {log_path}')
    with open(log_path, 'w') as logfid:
        pid = subprocess.Popen(cmd, stdin=subprocess.DEVNULL, stdout=logfid, stderr=logfid)
    print(f'{cmd_name} PID: {pid.pid}')
    return pid

def configure_prom(prom_dir, hosts, client_port):
    # Configure Prometheus server with targets
    targets = [f'{hh}:{client_port}' for hh in hosts]
    scrape_interval = 15
    evaluation_interval = 15
    prom_config = f"""\
global:
  scrape_interval: {scrape_interval}s
  evaluation_interval: {evaluation_interval}s
scrape_configs:
  - job_name: "prometheus"
    static_configs:
      - targets: {targets}
"""
    with open(f'{prom_dir}/prometheus.yml', 'w') as fid:
        fid.write(prom_config)

def configure_graf(graf_dir, graf_port):
    # Configure port for Grafana server
    graf_conf_path = f'{graf_dir}/conf/defaults.ini'
    with open(graf_conf_path, 'r') as fid:
        graf_conf = fid.read()
    graf_conf = re.sub(r'http_port = [0-9]*', f'http_port = {graf_port}', graf_conf)
    with open(graf_conf_path, 'w') as fid:
        fid.write(graf_conf)
    return graf_conf_path

def print_log(cmd_name, log_path):
    cmd_name = cmd_name.upper()
    print(f'{cmd_name} LOG:\n')
    with open(log_path, 'r') as fid:
        print(fid.read())

def create_hostfile(hosts):
    with NamedTemporaryFile('w', delete=False) as fid:
        fid.write('\n'.join(hosts))
        result = fid.name
    return result


def _term_pid(pid):
    if pid is None:
        return
    try:
        os.kill(pid.pid, signal.SIGINT)
        sys.stderr.write(f'Forwarded SIGINT to PID: {pid.pid}\n')
    except (ProcessLookupError, PermissionError):
        pass

_clush_pid = None
_prom_pid = None
_graf_pid = None
_temp_hostfile_path = None
def _signal_handler(signum, frame):
    sys.stderr.write('\n')
    for pid in (_clush_pid, _prom_pid, _graf_pid):
        _term_pid(pid)
    if _temp_hostfile_path is not None:
        os.unlink(_temp_hostfile_path)
    exit(0)

def run(prom_dir, graf_dir, prom_port, graf_port, client_port, geopm_dir, pbs_jobid, hostfile_path):
    """Entry function with inputs derived from CLI

    """
    # Timestamp for log files
    date_str = datetime.datetime.now(datetime.timezone.utc).astimezone().isoformat()

    # Check for Prometheus and Grafana executables
    prom_path = f'{prom_dir}/prometheus'
    graf_path = f'{graf_dir}/bin/grafana'
    if not os.path.exists(prom_path):
        raise RuntimeError(f'Prometheus directory does not contain prometheus executable: {prom_path}')
    if not os.path.exists(graf_path):
        raise RuntimeError(f'Grafana directory does not contain grafana executable: {graf_path}')

    # Create log and data file directories
    prom_log_dir = f'{prom_dir}/logs'
    graf_log_dir = f'{graf_dir}/logs'
    prom_data_dir = f'{prom_dir}/data'
    graf_data_dir = f'{graf_dir}/data'
    for dd in (prom_log_dir, prom_data_dir, graf_log_dir, graf_data_dir):
        os.makedirs(dd, exist_ok=True)

    clush_log_path = f'{prom_log_dir}/prometheus-client-{date_str}.log'
    prom_log_path = f'{prom_log_dir}/prometheus-server-{date_str}.log'
    graf_log_path = f'{graf_log_dir}/grafana-server-{date_str}.log'

    # Determine target hosts if tracking a job
    hosts = []
    global _clush_pid, _prom_pid, _graf_pid, _temp_hostfile_path
    signal.signal(signal.SIGINT, _signal_handler)

    if None not in (pbs_jobid, hostfile_path):
        raise RuntimeError('Both pbs_jobid and hostfile_path cannot both be specified')
    if pbs_jobid is not None:
        hosts = pbs_host_list(pbs_jobid)
        _temp_hostfile_path = create_hostfile(hosts)
        hostfile_path = _temp_hostfile_path
    elif hostfile_path is not None:
        with open(hostfile_path) as fid:
            hosts = [ll.strip() for ll in fid.readlines() if (len(ll.strip()) != 0 and ll.strip()[0] != '#')]
    # Launch geopmexporter on tracked hosts
    if len(hosts) != 0:
        clush_cmd = ['clush', f'--hostfile={hostfile_path}', '--',
                     'env', f'LD_LIBRARY_PATH={geopm_dir}/lib:{geopm_dir}/lib64:${{LD_LIBRARY_PATH}}',
                     'geopmexporter', '-p', f'{client_port}']
        _clush_pid = popen_wrapper(clush_cmd, 'clush', clush_log_path)

    configure_prom(prom_dir, hosts, client_port)
    graf_conf_path = configure_graf(graf_dir, graf_port)

    # Launch Prometheus server
    prom_cmd = [prom_path,
                f'--config.file={prom_dir}/prometheus.yml',
                f'--storage.tsdb.path={prom_dir}/data',
                f'--web.console.templates={prom_dir}/consoles',
                f'--web.console.libraries={prom_dir}/console_libraries',
                f'--web.listen-address=:{prom_port}']
    _prom_pid = popen_wrapper(prom_cmd, 'prometheus', prom_log_path)
    # Launch Grafana server
    graf_cmd = [graf_path, 'server',
                f'--config={graf_conf_path}',
                f'--homepath={graf_dir}',
                f'cfg:default.paths.logs={graf_dir}/logs',
                f'cfg:default.paths.data={graf_dir}/data',
                f'cfg:default.paths.plugins={graf_dir}/plugins-bundled',
                f'cfg:default.paths.provisioning={graf_dir}/conf/provisioning']
    _graf_pid = popen_wrapper(graf_cmd, 'grafana', graf_log_path)
    # Finish up
    if _clush_pid is not None:
        _clush_pid.wait()
        print_log('clush', clush_log_path)
        if _temp_hostfile_path is not None:
            os.unlink(_temp_hostfile_path)
    else:
        input('Press enter to kill prometheus and grafana servers: ')
    os.kill(signal.SIGINT, _graf_pid.pid)
    _graf_pid.wait()
    os.kill(signal.SIGINT, _prom_pid.pid)
    _prom_pid.wait()
    print_log('prometheus', prom_log_path)
    print_log('grafana', graf_log_path)

def main():
    """Monitor a set of hosts with geopmexporter using clush.

    Script will launch a Prometheus server and Grafana server on the login node.
    To inspect historically recorded data, run without the
    --pbs-jobid/--hostfile option.  The user will be prompted to press ENTER to
    terminate the servers.  To exit without printing the logs press Control-C
    instead of ENTER.

    In order to collect new data with this script, provide the
    --pbs-jobid/--hostfile option to run the geopmexporter Prometheus Client on
    the compute nodes of an PBS job allocation while the allocation is valid.
    When the PBS allocation terminates the two servers and the client process
    will also be terminated.

    """
    parser = ArgumentParser(description=main.__doc__)
    parser.add_argument('PROMETHEUS_DIR',
                        help='Directory containing Prometheus software')
    parser.add_argument('GRAFANA_DIR',
                        help='Directory containing Grafana software')
    parser.add_argument('--prom-port', type=int, default=9090,
                        help='Port for Prometheus server')
    parser.add_argument('--graf-port', type=int, default=3000,
                        help='Port for Grafanas server')
    parser.add_argument('--client-port', type=int, default=8000,
                        help='Port for geopmexporter Prometheus client')
    parser.add_argument('--geopm-prefix', default='/usr',
                        help='Path pprefix for user install of GEOPM')
    host_group = parser.add_mutually_exclusive_group()
    host_group.add_argument('--pbs-jobid', type=str, default=None,
                            help='PBS job ID to monitor')
    host_group.add_argument('--hostfile', type=str, default=None,
                            help='File with one hostname to monitor on each line')
    args = parser.parse_args()

    run(args.PROMETHEUS_DIR, args.GRAFANA_DIR, args.prom_port, args.graf_port,
        args.client_port, args.geopm_prefix, args.pbs_jobid, args.hostfile)
    return 0

if __name__ == '__main__':
    sys.exit(main())
