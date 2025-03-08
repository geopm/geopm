#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#



import unittest

import geopmdpy.hash
from geopmdpy import gffi


class TestHash(unittest.TestCase):
    def test_hash(self):
        hash_val = geopmdpy.hash.hash_str('abcdefg')
        self.assertEqual(geopmdpy.hash.hash_str('abcdefg'), hash_val)
        hash_val = geopmdpy.hash.hash_str('MPI_Bcast')
        self.assertEqual(geopmdpy.hash.hash_str('MPI_Bcast'), hash_val)

if __name__ == '__main__':
    unittest.main()
