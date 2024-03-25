#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#
import subprocess
import unittest
import time

from geopmdpy import pio
from geopmdpy import topo

from integration.test import geopm_test_launcher
from integration.test import util

@util.skip_unless_msr_access(msg='Requires test runner to hold the service write lock.  This breaks other tests.')
@util.skip_unless_batch()
class TestIntegrationProgrammableCounters(unittest.TestCase):
    '''Tests of programmable counters.'''

    def test_geopmread_command_line(self):
        '''Test that the count of retired instructions matches between the
        fixed counter and the programmable counter.
        '''
        fixed_signal_index = pio.push_signal('CPU_INSTRUCTIONS_RETIRED', topo.DOMAIN_BOARD, 0)

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
            total_fixed_count = pio.read_signal('CPU_INSTRUCTIONS_RETIRED', topo.DOMAIN_BOARD, 0) - initial_fixed_count
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
