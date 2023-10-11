#!/usr/bin/env python3
#
#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

"""Test that the gen_pbs.sh script generates PBS launch scripts with proper PBS directives and
invokes the correct experiment script based on input parameters.

"""

import sys
import unittest
import subprocess
import os


class TestIntegration_gen_pbs_positive(unittest.TestCase):
    TIME_LIMIT = 2
    NUM_NODES = '2'
    APP = 'app'
    EXP_DIR = 'dir'
    EXP_TYPE = 'type'

    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')

        cls._work_dir = os.environ['GEOPM_WORKDIR']
        cls._src_dir = os.environ['GEOPM_SOURCE']

        assert cls._work_dir
        assert cls._src_dir

        cls._gen_script_path = os.path.join(cls._src_dir, 'integration/experiment/gen_pbs.sh')

    def tearDown(self):
        try:
            os.remove('test.pbs')
        except FileNotFoundError:
            pass

    def test_all_args(self):
        self._run_script(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)
        self._check_script_contents(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)

    def test_no_exp_type(self):
        exp_type = self.EXP_DIR

        self._run_script(self.NUM_NODES, self.APP, self.EXP_DIR)
        self._check_script_contents(self.NUM_NODES, self.APP, self.EXP_DIR, exp_type)

    def test_accepts_user_account(self):
        user_account = 'test'
        os.environ['GEOPM_USER_ACCOUNT'] = user_account

        self._run_script(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)
        self._check_script_contents(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)
        self.assertIn(f'#PBS -A {user_account}', self._script_contents)

    def test_accepts_default_queue(self):
        default_queue = 'geopm_queue'
        os.environ['GEOPM_SYSTEM_DEFAULT_QUEUE'] = default_queue

        self._run_script(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)
        self._check_script_contents(self.NUM_NODES, self.APP, self.EXP_DIR, self.EXP_TYPE)
        self.assertIn(f'#PBS -q {default_queue}', self._script_contents)

    def _check_script_contents(self, num_nodes, app, exp_dir, exp_type):
        self.assertIn(f'#PBS -l nodes={num_nodes}', self._script_contents)
        self.assertIn(f'#PBS -o {self._work_dir}/{app}_{exp_type}.out',
                      self._script_contents)
        self.assertIn(f'#PBS -N {app}_{exp_type}', self._script_contents)
        self.assertIn('#PBS -l walltime', self._script_contents)

        self.assertIn(f'{self._src_dir}/integration/experiment/{exp_dir}/'
                      f'run_{exp_type}_{app}.py',
                      self._script_contents)
        self.assertIn(f'--node-count={num_nodes}', self._script_contents)
        self.assertIn('OUTPUT_DIR=${GEOPM_WORKDIR}/${PBS_JOBNAME}_${PBS_JOBID}',
                      self._script_contents)
        self.assertIn('--output-dir=${OUTPUT_DIR}', self._script_contents)

    def _run_script(self, *params):
        subprocess.run([self._gen_script_path, *params],
                       timeout=self.TIME_LIMIT, check=True)

        with open('test.pbs', 'r') as f:
            self._script_contents = f.read()


class TestIntegration_gen_pbs_negative(unittest.TestCase):
    TIME_LIMIT = 2

    @classmethod
    def setUpClass(cls):
        sys.stdout.write('(' + os.path.basename(__file__).split('.')[0] +
                         '.' + cls.__name__ + ') ...')

        cls._work_dir = os.environ['GEOPM_WORKDIR']
        cls._src_dir = os.environ['GEOPM_SOURCE']

        assert cls._work_dir
        assert cls._src_dir

        cls._gen_script_path = os.path.join(cls._src_dir, 'integration/experiment/gen_pbs.sh')

    def test_no_args(self):
        cp = self._run_script()
        self._check_script_result(cp)

    def test_too_few_args(self):
        cp = self._run_script('1', 'app')
        self._check_script_result(cp)

    def test_too_many_args(self):
        cp = self._run_script('1', 'app', 'exp_dir', 'exp_type', 'extra_arg')
        self._check_script_result(cp)

    def _check_script_result(self, cp):
        self.assertEqual(cp.returncode, 1)
        self.assertIn('Usage', cp.stdout.decode())

    def _run_script(self, *params):
        return subprocess.run([self._gen_script_path, *params], stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, timeout=self.TIME_LIMIT)


if __name__ == '__main__':
    unittest.main()
