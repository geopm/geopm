#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import io
import sys
import unittest
from unittest.mock import patch

import geopmdpy.read as read


class TestReadCLI(unittest.TestCase):
    def setUp(self):
        self._saved_argv = list(sys.argv)
        self._stdout = io.StringIO()
        self._stderr = io.StringIO()
        self._patch_stdout = patch('sys.stdout', new=self._stdout)
        self._patch_stderr = patch('sys.stderr', new=self._stderr)
        self._patch_stdout.start()
        self._patch_stderr.start()

    def tearDown(self):
        self._patch_stdout.stop()
        self._patch_stderr.stop()
        sys.argv = self._saved_argv

    def test_default_print_signals(self):
        sys.argv = ['prog']
        with patch('geopmdpy.read.pio.signal_names', return_value=['s1', 's2']):
            rc = read.run()
        self.assertEqual(0, rc)
        self.assertEqual('s1\ns2\n', self._stdout.getvalue())

    def test_domain_flag_dispatches(self):
        sys.argv = ['prog', '-d']
        with patch('geopmdpy.read.print_domains') as p_dom:
            rc = read.run()
        self.assertEqual(0, rc)
        p_dom.assert_called_once()

    def test_print_domains_content(self):
        mapping = {
            'board': 1,
            'package': 2,
            'core': 3,
            'cpu': 4,
            'memory': 5,
            'package_integrated_memory': 6,
            'nic': 7,
            'package_integrated_nic': 8,
            'gpu': 9,
            'package_integrated_gpu': 10,
            'gpu_chip': 11,
        }
        def _num_domain(name):
            return mapping[name]
        with patch('geopmdpy.read.topo.num_domain', side_effect=_num_domain):
            read.print_domains()
        self.assertIn('board                       1', self._stdout.getvalue())
        self.assertIn('gpu_chip                    11', self._stdout.getvalue())

    def test_signal_domain_print(self):
        sys.argv = ['prog', '-D', 'signal_x']
        with patch('geopmdpy.read.pio.signal_domain_type', return_value=3), \
             patch('geopmdpy.read.topo.domain_name', return_value='cpu'):
            rc = read.run()
        self.assertEqual(0, rc)
        self.assertEqual('cpu\n', self._stdout.getvalue())

    def test_info_print(self):
        sys.argv = ['prog', '-i', 'signal_x']
        with patch('geopmdpy.read.pio.signal_description', return_value='desc-x'):
            rc = read.run()
        self.assertEqual(0, rc)
        self.assertEqual('signal_x:\ndesc-x\n', self._stdout.getvalue())

    def test_info_all_print(self):
        sys.argv = ['prog', '-I']
        with patch('geopmdpy.read.pio.signal_names', return_value=['s1', 's2']), \
             patch('geopmdpy.read.pio.signal_description', side_effect=lambda n: f'desc-{n}'):
            rc = read.run()
        self.assertEqual(0, rc)
        out = self._stdout.getvalue()
        self.assertIn('s1:\ndesc-s1\n', out)
        self.assertIn('s2:\ndesc-s2\n', out)

    def test_cache_creation(self):
        sys.argv = ['prog', '-c']
        with patch('geopmdpy.read.topo.create_cache') as p_cache:
            rc = read.run()
        self.assertEqual(0, rc)
        p_cache.assert_called_once()

    def test_positional_read(self):
        sys.argv = ['prog', 'SIG_A', 'cpu', '3']
        with patch('geopmdpy.read.pio.read_signal', return_value=12.34) as p_read, \
             patch('geopmdpy.read.pio.signal_info', return_value=('units', 'fmt')) as p_info, \
             patch('geopmdpy.read.pio.format_signal', return_value='12.34 W'):
            rc = read.run()
        self.assertEqual(0, rc)
        p_read.assert_called_once_with('SIG_A', 'cpu', 3)
        p_info.assert_called_once_with('SIG_A')
        self.assertEqual('12.34 W\n', self._stdout.getvalue())

    def test_invalid_domain_index_error_main(self):
        sys.argv = ['prog', 'SIG_A', 'cpu', 'not-an-int']
        rc = read.main()
        self.assertEqual(-1, rc)
        self.assertTrue(self._stderr.getvalue().startswith('Error: invalid domain index: not-an-int'))

if __name__ == '__main__':
    unittest.main()
