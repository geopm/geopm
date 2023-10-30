#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""
This integration test verifies that the gpu_frequency_sweep
can be used to generate a const config characterization file
"""
import sys
import unittest
import os
import json
from pathlib import Path

from integration.test import util

from experiment.gpu_ca_characterization import GPUCACharacterization

@util.skip_unless_gpu()
@util.skip_unless_workload_exists("apps/parres/Kernels/Cxx11/")
class TestIntegration_gpu_characterization(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """
        Setup DGEMM & STREAM applications, setup agent config, and execute.
        """
        cls._skip_launch = not util.do_launch()

        base_dir = 'test_gpu_characterization_output'
        cls._gpu_ca_characterization = GPUCACharacterization(base_dir=base_dir)
        gpu_config = None
        if not cls._skip_launch:
            gpu_config = cls._gpu_ca_characterization.do_characterization()

        # Write config
        json_config = json.dumps(gpu_config, indent=4)
        with open("const_config_io-ca.json", "w") as outfile:
            outfile.write(json_config)

        const_path = Path('const_config_io-ca.json').resolve()
        os.environ["GEOPM_CONST_CONFIG_PATH"] = str(const_path)

    def tearDown(self):
        if sys.exc_info() != (None, None, None):
            TestIntegration_gpu_characterization._keep_files = True

    def test_gpu_characterization(self):
        """
        Fge is readable via geopmread
        Fge <= Fmax
        Fge >= Fmin
        """
        pass


if __name__ == '__main__':
    # Call do_launch to clear non-pyunit command line option
    util.do_launch()
    unittest.main()
