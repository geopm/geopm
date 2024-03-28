#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#



import unittest
from importlib import reload

import geopmpy.hash
from geopmdpy import gffi


class TestHash(unittest.TestCase):
    def setUp(self):
        # Ensures that mocks do not leak into this test
        reload(gffi)
        reload(geopmpy.hash)

    def test_hash(self):
        hash_val = geopmpy.hash.hash_str('abcdefg')
        self.assertEqual(geopmpy.hash.hash_str('abcdefg'), hash_val)
        hash_val = geopmpy.hash.hash_str('MPI_Bcast')
        self.assertEqual(geopmpy.hash.hash_str('MPI_Bcast'), hash_val)

if __name__ == '__main__':
    unittest.main()
