#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
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
        hash = geopmpy.hash.hash_str('abcdefg')
        self.assertEqual(0x312a6aa6, hash)
        hash = geopmpy.hash.hash_str('MPI_Bcast')
        self.assertEqual(0xa176e473, hash)

if __name__ == '__main__':
    unittest.main()
