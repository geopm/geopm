#!/usr/bin/env python3
#
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
import subprocess
import unittest
import geopmdpy.pio as pio
import geopmdpy.topo as topo
import time

import geopm_context
import geopm_test_launcher
import util

@util.skip_unless_batch()
class TestIntegrationProgrammableCounters(unittest.TestCase):
    '''Tests of programmable counters.'''

    def test_geopmread_command_line(self):
        '''Test that the count of retired instructions matches between the
        fixed counter and the programmable counter.
        '''
        fixed_signal_index = pio.push_signal('INSTRUCTIONS_RETIRED', topo.DOMAIN_BOARD, 0)

        # See https://download.01.org/perfmon/SKX/skylakex_core_v1.24.json
        # for EVENT_SELECT and UMASK for INST_RETIRED.ANY_P
        pio.write_control('MSR::IA32_PERFEVTSEL0:EVENT_SELECT', topo.DOMAIN_BOARD, 0, 0xc0)
        pio.write_control('MSR::IA32_PERFEVTSEL0:UMASK', topo.DOMAIN_BOARD, 0, 0)
        pio.write_control('MSR::IA32_PERFEVTSEL0:USR', topo.DOMAIN_BOARD, 0, 1)
        pio.write_control('MSR::IA32_PERFEVTSEL0:OS', topo.DOMAIN_BOARD, 0, 1)
        pio.write_control('MSR::IA32_PERFEVTSEL0:EN', topo.DOMAIN_BOARD, 0, 1)
        pio.write_control('MSR::PERF_GLOBAL_CTRL:EN_PMC0', topo.DOMAIN_BOARD, 0, 1)
        pmc_signal_index = pio.push_signal('MSR::IA32_PMC0:PERFCTR', topo.DOMAIN_BOARD, 0)

        timeout = time.time() + 15

        pio.read_batch()
        initial_fixed_count = pio.sample(fixed_signal_index)
        initial_pmc_count = pio.sample(pmc_signal_index)

        while time.time() < timeout:
            # Reproduce https://github.com/geopm/geopm/issues/1931
            # The purpose of this loop is to trigger the conditions that
            # reproduce the bug. Don't use read_batch here, since geopm's
            # rollover handling may mask the problem intermittently.
            total_fixed_count = pio.read_signal('INSTRUCTIONS_RETIRED', topo.DOMAIN_BOARD, 0) - initial_fixed_count
            # When the bug is present, the PMC will be truncated at 32 bits,
            # for values up to 48 bits.
            if total_fixed_count > (1 << 33):
                break;

        # Perform the final test with read_batch() instead of individual read
        # calls, to decrease the number of instructions that execute between
        # reading the counters.
        pio.read_batch()
        total_fixed_count = pio.sample(fixed_signal_index) - initial_fixed_count
        total_pmc_count = pio.sample(pmc_signal_index) - initial_pmc_count

        self.assertAlmostEqual(total_fixed_count, total_pmc_count,
                               delta=0.01 * total_fixed_count)

if __name__ == '__main__':
    unittest.main()
