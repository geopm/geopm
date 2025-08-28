#!/usr/bin/env python3
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import io
import os
import sys
import tempfile
import unittest
from unittest.mock import patch, call

import geopmdpy.write as write


class TestWriteCLI(unittest.TestCase):
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

    def test_default_print_controls(self):
        sys.argv = ['prog']
        with patch('geopmdpy.write.pio.control_names', return_value=['c1', 'c2']):
            rc = write.run()
        self.assertEqual(0, rc)
        self.assertEqual('c1\nc2\n', self._stdout.getvalue())

    def test_domain_flag_dispatches_to_read(self):
        sys.argv = ['prog', '-d']
        with patch('geopmdpy.write.read.print_domains') as p_dom:
            rc = write.run()
        self.assertEqual(0, rc)
        p_dom.assert_called_once()

    def test_control_domain_print(self):
        sys.argv = ['prog', '-D', 'CTRL_X']
        with patch('geopmdpy.write.pio.control_domain_type', return_value=2), \
             patch('geopmdpy.write.topo.domain_name', return_value='package'):
            rc = write.run()
        self.assertEqual(0, rc)
        self.assertEqual('package\n', self._stdout.getvalue())

    def test_info_print(self):
        sys.argv = ['prog', '-i', 'CTRL_X']
        with patch('geopmdpy.write.pio.control_description', return_value='desc-x'):
            rc = write.run()
        self.assertEqual(0, rc)
        self.assertEqual('CTRL_X:\ndesc-x\n', self._stdout.getvalue())

    def test_info_all_print(self):
        sys.argv = ['prog', '-I']
        with patch('geopmdpy.write.pio.control_names', return_value=['c1', 'c2']), \
             patch('geopmdpy.write.pio.control_description', side_effect=lambda n: f'desc-{n}'):
            rc = write.run()
        self.assertEqual(0, rc)
        out = self._stdout.getvalue()
        self.assertIn('c1:\ndesc-c1\n', out)
        self.assertIn('c2:\ndesc-c2\n', out)

    def test_cache_creation(self):
        sys.argv = ['prog', '-c']
        with patch('geopmdpy.write.topo.create_cache') as p_cache:
            rc = write.run()
        self.assertEqual(0, rc)
        p_cache.assert_called_once()

    def test_enable_fixed_counters(self):
        sys.argv = ['prog', '-e']
        with patch('geopmdpy.write.pio.enable_fixed_counters') as p_en:
            rc = write.run()
        self.assertEqual(0, rc)
        p_en.assert_called_once()

    def test_positional_write(self):
        sys.argv = ['prog', 'CTRL_A', 'cpu', '3', '1.5']
        with patch('geopmdpy.write.pio.write_control') as p_write:
            rc = write.run()
        self.assertEqual(0, rc)
        p_write.assert_called_once_with('CTRL_A', 'cpu', 3, 1.5)

    def test_batch_from_file_success(self):
        content = "CTRL_A cpu 0 1.0\nCTRL_A cpu 1 2.0\n\n"
        with tempfile.NamedTemporaryFile('w+', delete=False) as tf:
            tf.write(content)
            tf.flush()
            path = tf.name
        try:
            sys.argv = ['prog', '-f', path]
            with patch('geopmdpy.write.pio.control_names', return_value=['CTRL_A']), \
                 patch('geopmdpy.write.topo.domain_type', return_value=0), \
                 patch('geopmdpy.write.topo.num_domain', return_value=8), \
                 patch('geopmdpy.write.pio.push_control', side_effect=['i0', 'i1']) as p_push, \
                 patch('geopmdpy.write.pio.adjust') as p_adjust, \
                 patch('geopmdpy.write.pio.write_batch') as p_batch:
                rc = write.run()
            self.assertEqual(0, rc)
            self.assertEqual([call('CTRL_A', 0, 0), call('CTRL_A', 0, 1)], p_push.call_args_list)
            self.assertEqual([call('i0', 1.0), call('i1', 2.0)], p_adjust.call_args_list)
            p_batch.assert_called_once()
        finally:
            os.unlink(path)

    def test_batch_from_stdin_success(self):
        sys.argv = ['prog', '-f', '-']
        data = io.StringIO("CTRL_A cpu 2 3.14\n")
        with patch('sys.stdin', new=data), \
             patch('geopmdpy.write.pio.control_names', return_value=['CTRL_A']), \
             patch('geopmdpy.write.topo.domain_type', return_value=0), \
             patch('geopmdpy.write.topo.num_domain', return_value=4), \
             patch('geopmdpy.write.pio.push_control', return_value='ix') as p_push, \
             patch('geopmdpy.write.pio.adjust') as p_adjust, \
             patch('geopmdpy.write.pio.write_batch') as p_batch:
            rc = write.run()
        self.assertEqual(0, rc)
        p_push.assert_called_once_with('CTRL_A', 0, 2)
        p_adjust.assert_called_once_with('ix', 3.14)
        p_batch.assert_called_once()

    def test_batch_invalid_line_error_main_too_few(self):
        with tempfile.NamedTemporaryFile('w+', delete=False) as tf:
            tf.write("too few tokens\n")
            tf.flush()
            path = tf.name
        try:
            sys.argv = ['prog', '-f', path]
            rc = write.main()
            self.assertEqual(-1, rc)
            self.assertIn('Number of words per line in configuration file must be 4', self._stderr.getvalue())
        finally:
            os.unlink(path)

    def test_batch_invalid_line_error_main_too_many(self):
        with tempfile.NamedTemporaryFile('w+', delete=False) as tf:
            tf.write("too many tokens per line\n")
            tf.flush()
            path = tf.name
        try:
            sys.argv = ['prog', '-f', path]
            rc = write.main()
            self.assertEqual(-1, rc)
            self.assertIn('Number of words per line in configuration file must be 4', self._stderr.getvalue())
        finally:
            os.unlink(path)

    def test_batch_unknown_control_main(self):
        with tempfile.NamedTemporaryFile('w+', delete=False) as tf:
            tf.write("UNKNOWN cpu 0 1.0\n")
            tf.flush()
            path = tf.name
        try:
            sys.argv = ['prog', '-f', path]
            with patch('geopmdpy.write.pio.control_names', return_value=['CTRL_A']):
                rc = write.main()
            self.assertEqual(-1, rc)
            self.assertIn('Control name unknown: UNKNOWN', self._stderr.getvalue())
        finally:
            os.unlink(path)

    def test_batch_domain_index_out_of_bounds_main(self):
        with tempfile.NamedTemporaryFile('w+', delete=False) as tf:
            tf.write("CTRL_A cpu 9 1.0\n")
            tf.flush()
            path = tf.name
        try:
            sys.argv = ['prog', '-f', path]
            with patch('geopmdpy.write.pio.control_names', return_value=['CTRL_A']), \
                 patch('geopmdpy.write.topo.domain_type', return_value=0), \
                 patch('geopmdpy.write.topo.num_domain', return_value=2):
                rc = write.main()
            self.assertEqual(-1, rc)
            self.assertIn('Domain index out of bounds: 9', self._stderr.getvalue())
        finally:
            os.unlink(path)

if __name__ == '__main__':
    unittest.main()
