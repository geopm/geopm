#!/usr/bin/env python3

#  Copyright (c) 2015 - 2022, Intel Corporation
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

import sys
import geopmdpy.runtime
import geopmdpy.topo
import geopmpy.reporter

class LocalAgent(geopmdpy.runtime.Agent):
    """Simple example that uses the geopmdpy.runtime module

    Runs a command while executing a five millisecond control loop. On
    each control loop step the agent prints out time and instructions
    retired for each package. The agent also sets the CPU frequency
    cap based on the policy.  If the policy is None the CPU frequency
    is unlimited.

    """
    def __init__(self):
        self._loop_idx = 0
        self._cpu_freq = geopmdpy.pio.read_signal('CPU_FREQUENCY_MAX', 'board', 0)
        geopmpy.reporter.init()

    def get_signals(self):
        result = [("TIME", geopmdpy.topo.DOMAIN_BOARD, 0)]
        for pkg in range(geopmdpy.topo.num_domain(geopmdpy.topo.DOMAIN_PACKAGE)):
            result.append(("INSTRUCTIONS_RETIRED", geopmdpy.topo.DOMAIN_PACKAGE, pkg))
        return result

    def get_controls(self):
        return [("CPU_FREQUENCY_CONTROL", geopmdpy.topo.DOMAIN_BOARD, 0)]

    def run_begin(self, policy):
        if policy is not None:
            if policy < 0 or policy > self._cpu_freq:
                raise RuntimeError(f'Invalid CPU frequency value: {policy}')
            self._cpu_freq = policy

    def run_end(self):
        self._loop_idx = 0
        self._cpu_freq = geopmdpy.pio.read_signal('CPU_FREQUENCY_MAX', 'board', 0)

    def update(self, signals):
        if self._loop_idx == 0:
            self._signals_begin = list(signals)
        self._signals_last = list(signals)
        print(signals)
        self._loop_idx += 1
        geopmpy.reporter.update()
        return [self._cpu_freq]

    def get_period(self):
        return 0.005

    def get_report(self, profile):
        return geopmpy.reporter.generate(profile, 'test_agent')

def main():
    """Run the LocalAgent

    """
    err = 0
    help_msg = f'Usage: {sys.argv[0]} CPU_FREQUENCY_MAX COMMAND\n\n{LocalAgent.__doc__}\n'
    if len(sys.argv) < 3:
        err = -1
    if err != 0 or sys.argv[1] == '--help':
        sys.stderr.write(help_msg)
        return err
    cpu_frequency_max = float(sys.argv[1])
    agent = LocalAgent()
    controller = geopmdpy.runtime.Controller(agent)
    report = controller.run(sys.argv[2:], cpu_frequency_max, profile='test_profile_name')
    with open('geopm.report', 'w') as fid:
        fid.write(report)
    return controller.returncode()

if __name__ == '__main__':
    exit(main())
