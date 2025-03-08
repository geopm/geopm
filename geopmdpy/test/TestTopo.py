#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#


import unittest
from geopmdpy import topo
from geopmdpy import gffi

class TestTopo(unittest.TestCase):
    def test_num_domain(self):
        num_cpu_str = topo.num_domain("cpu")
        self.assertLess(0, num_cpu_str)
        num_cpu_int = topo.num_domain(topo.DOMAIN_CPU)
        self.assertEqual(num_cpu_str, num_cpu_int)

    def test_domain_domain_nested(self):
        num_cpu = topo.num_domain("cpu")
        num_pkg = topo.num_domain("package")
        pkg_cpu_map = [[] for pkg_idx in range(num_pkg)]
        for cpu_idx in range(num_cpu):
            pkg_idx_str = topo.domain_idx("package", cpu_idx)
            pkg_idx_int = topo.domain_idx(topo.DOMAIN_PACKAGE, cpu_idx)
            self.assertEqual(pkg_idx_int, pkg_idx_str)
            pkg_cpu_map[pkg_idx_str].append(cpu_idx)
        for pkg_idx in range(num_pkg):
            self.assertEqual(pkg_cpu_map[pkg_idx], topo.domain_nested('cpu', 'package', pkg_idx))
            self.assertEqual(pkg_cpu_map[pkg_idx], topo.domain_nested(topo.DOMAIN_CPU, topo.DOMAIN_PACKAGE, pkg_idx))

    def test_domain_name_type(self):
        self.assertEqual('cpu', topo.domain_name(topo.DOMAIN_CPU))
        self.assertEqual(topo.DOMAIN_CPU, topo.domain_type('cpu'))
        with self.assertRaises(RuntimeError):
            topo.num_domain('non-domain')
        with self.assertRaises(RuntimeError):
            topo.num_domain(100)

if __name__ == '__main__':
    unittest.main()
