#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2026 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import unittest
from unittest import mock
from argparse import ArgumentParser
from argparse import Namespace

# Patch dlopen to allow the tests to run when there is no build
with mock.patch('cffi.FFI.dlopen', return_value=mock.MagicMock()):
    from geopmdpy import monitor_agent
    from geopmdpy.monitor_agent import MonitorAgent


class TestMonitorAgent(unittest.TestCase):
    def setUp(self):
        self._test_name = 'TestMonitorAgent'

    def test_help_nonempty(self):
        self.assertIsInstance(MonitorAgent().help(), str)
        self.assertTrue(MonitorAgent().help())

    def test_update_parser_adds_hi_res(self):
        agent = MonitorAgent()
        parser = agent.update_parser(ArgumentParser())
        self.assertTrue(parser.parse_args(['--hi-res']).hi_res)
        self.assertFalse(parser.parse_args([]).hi_res)

    def test_update_args_sets_hi_res(self):
        agent = MonitorAgent()
        agent.update_args(Namespace(hi_res=True))
        self.assertTrue(agent._hi_res)
        agent.update_args(Namespace(hi_res=False))
        self.assertFalse(agent._hi_res)

    def test_signal_config_override_default(self):
        requests = [('CPU_POWER', 0, 0), ('CPU_ENERGY', 0, 0)]
        agent = MonitorAgent()
        agent.update_args(Namespace(hi_res=False))
        with mock.patch.object(monitor_agent, 'default_requests',
                               return_value=requests):
            override = agent.signal_config_override()
        self.assertEqual('TIME board 0\n'
                         'CPU_POWER board 0\n'
                         'CPU_ENERGY board 0\n', override)

    def test_signal_config_override_hi_res(self):
        requests = [('CPU_POWER', 0, 0), ('CPU_ENERGY', 0, 0)]
        agent = MonitorAgent()
        agent.update_args(Namespace(hi_res=True))
        with mock.patch.object(monitor_agent, 'default_requests',
                               return_value=requests):
            override = agent.signal_config_override()
        self.assertEqual('TIME board 0\n'
                         'CPU_POWER * *\n'
                         'CPU_ENERGY * *\n', override)

    def test_signal_config_override_single_signal(self):
        agent = MonitorAgent()
        agent.update_args(Namespace(hi_res=False))
        with mock.patch.object(monitor_agent, 'default_requests',
                               return_value=[('CPU_POWER', 0, 0)]):
            override = agent.signal_config_override()
        self.assertEqual('TIME board 0\nCPU_POWER board 0\n', override)


if __name__ == '__main__':
    unittest.main()
