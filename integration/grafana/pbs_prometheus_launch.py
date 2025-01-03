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
from getpass import getuser
from argparse import ArgumentParser
from time import sleep
from tempfile import NamedTemporaryFile

def get_host_list(jobid):
    """Get list of hostnames from a PBS jobid

    """
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
            sleep(1)
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


def run(prom_dir, graf_dir, prom_port, graf_port, client_port, geopm_dir, jobid):
    """Entry function with inputs derived from CLI

    """
    date_str = datetime.datetime.now(datetime.timezone.utc).astimezone().isoformat()
    # Check for Prometheus and Grafana executables
    prom_path = f'{prom_dir}/prometheus'
    graf_path = f'{graf_dir}/bin/grafana'
    if not os.path.exists(prom_path):
        raise RuntimeError(f'Prometheus directory does not contain prometheus executable: {prom_path}')
    if not os.path.exists(graf_path):
        raise RuntimeError(f'Grafana directory does not contain grafana executable: {graf_path}')
    # Create log and data file directories
    os.makedirs('{prom_dir}/logs', exist_ok=True)
    os.makedirs('{prom_dir}/data', exist_ok=True)
    os.makedirs('{graf_dir}/logs', exist_ok=True)
    os.makedirs('{graf_dir}/data', exist_ok=True)

    targets = []
    # Determine target hosts if tracking a job
    if jobid is not None:
        hosts = get_host_list(jobid)
        with NamedTemporaryFile('w', delete=False) as fid:
            fid.write('\n'.join(hosts))
            hostfile_path = fid.name
        # Launch geopmexporter on tracked hosts
        clush_cmd = ['clush', f'--hostfile={hostfile_path}', '--',
                     'env', f'LD_LIBRARY_PATH={geopm_dir}/lib:{geopm_dir}/lib64:${{LD_LIBRARY_PATH}}',
                     'geopmexporter', '-p', f'{client_port}']
        clush_log_path = f'{prom_dir}/logs/prometheus-client-{date_str}.log'
        print(f'CLUSH COMMAND:      {" ".join(clush_cmd)}')
        with open(clush_log_path, 'w') as logfid:
            clush_pid = subprocess.Popen(clush_cmd, stdin=subprocess.DEVNULL, stdout=logfid, stderr=logfid)
        print(f'CLUSH PID:          {clush_pid.pid}')
        print(f'CLUSH LOGFILE:      {clush_log_path}')

        targets = [f'{hh}:{client_port}' for hh in hosts]
    # Configure Prometheus server with targets
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
    # Configure port for Grafana server
    graf_conf_path = f'{graf_dir}/conf/defaults.ini'
    with open(graf_conf_path, 'r') as fid:
        graf_conf = fid.read()
    graf_conf = re.sub(r'http_port = [0-9]*', f'http_port = {graf_port}', graf_conf)
    with open(graf_conf_path, 'w') as fid:
        fid.write(graf_conf)
    # Launch Prometheus server
    prom_cmd = [prom_path,
                f'--config.file={prom_dir}/prometheus.yml',
                f'--storage.tsdb.path={prom_dir}/data',
                f'--web.console.templates={prom_dir}/consoles',
                f'--web.console.libraries={prom_dir}/console_libraries',
                f'--web.listen-address=:{prom_port}']
    prom_log_path = f'{prom_dir}/logs/prometheus-server-{date_str}.log'
    print(f'PROMETHEUS COMMAND: {" ".join(prom_cmd)}')
    print(f'PROMETHEUS LOGFILE: {prom_log_path}')
    with open(prom_log_path, 'w') as logfid:
        prom_pid = subprocess.Popen(prom_cmd, stdin=subprocess.DEVNULL, stdout=logfid, stderr=logfid)
    print(f'PROMETHEUS PID:     {prom_pid.pid}')
    # Launch Grafana server
    graf_log_path = f'{graf_dir}/logs/grafana-server-{date_str}.log'
    graf_cmd = [graf_path, 'server',
                f'--config={graf_conf_path}',
                f'--homepath={graf_dir}',
                f'cfg:default.paths.logs={graf_dir}/logs',
                f'cfg:default.paths.data={graf_dir}/data',
                f'cfg:default.paths.plugins={graf_dir}/plugins-bundled',
                f'cfg:default.paths.provisioning={graf_dir}/conf/provisioning']
    print(f'GRAFANA COMMAND:    {" ".join(graf_cmd)}')
    with open(graf_log_path, 'w') as logfid:
        graf_pid = subprocess.Popen(graf_cmd, stdin=subprocess.DEVNULL, stdout=logfid, stderr=logfid)
    print(f'GRAFANA PID:        {graf_pid.pid}')
    # Finsish up
    print(f'GRAFANA LOGFILE:    {graf_log_path}')
    if jobid is not None:
        clush_pid.wait()
        print("CLUSH LOG:\n")
        with open(clush_log_path, 'r') as fid:
            print(fid.read())
        os.unlink(hostfile_path)
    else:
        input('Press enter to kill prometheus and grafana servers: ')
    prom_pid.kill()
    graf_pid.kill()
    prom_pid.wait()
    print("PROMETHEUS LOG:\n")
    with open(prom_log_path, 'r') as fid:
        print(fid.read())
    graf_pid.wait()
    print("GRAFANA LOG:\n")
    with open(graf_log_path, 'r') as fid:
        print(fid.read())

def main():
    """Monitor PBS jobs with geopmexporter.

    Script will launch a Prometheus server and Grafana server on the login node.
    To inspect historically recorded data, run without the --jobid option.  The
    user will be prompted to press ENTER to terminate the servers.

    In order to collect new data with this script, provide the --jobid option to
    run the geopmexporter Prometheus Client on the compute nodes of an PBS job
    allocation while the allocation is valid.  When the PBS allocation
    terminates the two servers and the client process will also be terminated.

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
                        help='Port for geopmexporter Promethues client')
    parser.add_argument('--geopm-prefix', default='/usr',
                        help='Path pprefix for user install of GEOPM')
    parser.add_argument('--jobid', type=str, default=None,
                        help='PBS job ID to monitor')
    args = parser.parse_args()

    run(args.PROMETHEUS_DIR, args.GRAFANA_DIR, args.prom_port, args.graf_port,
        args.client_port, args.geopm_prefix, args.jobid)
    return 0

if __name__ == '__main__':
    sys.exit(main())
