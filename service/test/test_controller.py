#!/usr/bin/env python3

#  Copyright (c) 2015 - 2021, Intel Corporation
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


class LocalAgent(geopmdpy.runtime.Agent):
    """Simple example that uses the geopmdpy.runtime module

    Runs a command while executing a five millisecond control loop. On each
    control loop step the agent prints outtime and instructions retired.

    """
    def __init__(self):
        self._loop_idx = 0

    def get_signals(self):
        return [("TIME", geopmdpy.topo.DOMAIN_BOARD, 0),
                ("INSTRUCTIONS_RETIRED", geopmdpy.topo.DOMAIN_PACKAGE, 0)]

    def get_controls(self):
        return []

    def update(self, signals):
        if self._loop_idx == 0:
            self._signals_begin = list(signals)
        self._signals_last = list(signals)
        print(signals)
        self._loop_idx += 1
        return []

    def get_period(self):
        return 0.005

    def get_report(self):
        delta = [ee - bb for (bb, ee) in
                 zip(self._signals_begin,
                     self._signals_last)]
        return f'\n\nTotal time: {delta[0]}\nTotal instructions: {delta[1]}'


def main():
    """Run the LocalAgent

    """
    err = 0
    help_msg = f"""\
Usage: {sys.argv[0]} COMMAND

{LocalAgent.__doc__}
    """
    if len(sys.argv) < 2:
        err = -1
    if err != 0 or sys.argv[1] == '--help':
        sys.stderr.print(help_msg)
        return err
    agent = LocalAgent()
    controller = geopmdpy.runtime.Controller(agent, sys.argv[1:])
    print(controller.run())
    return err

if __name__ == '__main__':
    exit(main())
