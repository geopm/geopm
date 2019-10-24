#!/usr/bin/env python
# python3 compliant

import unittest
import subprocess
import re
import ast
import os
import sys

class TestAffinity(unittest.TestCase):
    _reserved_cores = 2
    _verbosity = 0

    def get_bound_cpus(self, cpus_per_rank, desired_ranks, geopm_enabled=False):
        mask_list = []

        for rank in range(desired_ranks):
            mask = hex(int('1' * cpus_per_rank, self._reserved_cores) << (cpus_per_rank * rank))
            if geopm_enabled:
                mask = hex(int(mask, 16) << self._reserved_cores) # Shift left 2 cores
            mask_list.append(mask)

        if geopm_enabled:
            mask_list.insert(0, hex(self._reserved_cores))
            desired_ranks += 1

        environ=os.environ.copy()
        environ.update({'MV2_ENABLE_AFFINITY' : '0',
                        'OMP_PROC_BIND' : 'true'})

        print_affinity='OMP_NUM_THREADS={} srun -N1 -n{} --cpu-bind v,mask_cpu:{}' \
                       ' -- ../examples/print_affinity'.format(
                        cpus_per_rank, desired_ranks, ','.join(mask_list))

        if self._verbosity == 1:
            sys.stdout.write('\n{}\n'.format(print_affinity))

        pid = subprocess.Popen(print_affinity, env=environ,
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                               shell=True)
        stdout_str, stderr_str = pid.communicate()
        stdout_str = stdout_str.decode()
        stderr_str = stderr_str.decode()

        if self._verbosity == 1:
            sys.stdout.write('\n{}\n'.format(stdout_str))
            sys.stdout.write('\nEr: {}\n'.format(stderr_str))

        if pid.returncode != 0:
            raise RuntimeError('{}\n'.format(print_affinity), stderr_str)

        rank_bind_list = list(filter(None, stdout_str.split('\n')))

        cgroup = []
        omp_cpus = []
        for line in rank_bind_list:
            match = re.search(r'.*(\[.*\]).*(\[.*\])', line)
            cgroup.append(ast.literal_eval(match.group(1)))
            omp_cpus.append(ast.literal_eval(match.group(2)))

        return cgroup, omp_cpus

    def check_core_groups(self, ranks, cpus_per_rank, cgroup, omp_cpus, geopm_enabled=False):
        total_ranks = ranks
        if geopm_enabled:
            total_ranks = ranks + 1

        for rank in range(total_ranks):
            if geopm_enabled and rank == 0:
                expected_cgroup = [1]
                expected_omp_cpus = expected_cgroup * cpus_per_rank
            else:
                real_rank = rank
                if geopm_enabled:
                    real_rank = rank - 1 # GEOPM will run on rank 0
                start_core = real_rank * cpus_per_rank
                end_core = cpus_per_rank + (real_rank * cpus_per_rank)
                if geopm_enabled:
                    start_core += self._reserved_cores
                    end_core += self._reserved_cores
                expected_cgroup = list(range(start_core, end_core))
                expected_omp_cpus = expected_cgroup

            self.assertEqual(cgroup[rank], expected_cgroup)
            self.assertEqual(sorted(omp_cpus[rank]), expected_omp_cpus)

    def check_affinity(self, ranks, cpus_per_rank):
        cgroup, omp_cpus = self.get_bound_cpus(cpus_per_rank, ranks)
        self.check_core_groups(ranks, cpus_per_rank, cgroup, omp_cpus)
        cgroup, omp_cpus = self.get_bound_cpus(cpus_per_rank, ranks, geopm_enabled=True)
        self.check_core_groups(ranks, cpus_per_rank, cgroup, omp_cpus, geopm_enabled=True)

    def test_1_rank_1_core(self):
        self.check_affinity(1, 1)

    def test_1_rank_4_cores(self):
        self.check_affinity(1, 4)

    def test_1_rank_10_cores(self):
        self.check_affinity(1, 10)

    def test_2_ranks_4_cores(self):
        self.check_affinity(2, 2)

    def test_3_ranks_9_cores(self):
        self.check_affinity(3, 3)

    def test_4_ranks_8_cores(self):
        self.check_affinity(4, 2)

    def test_4_ranks_16_cores(self):
        self.check_affinity(4, 4)

    def test_8_ranks_8_cores(self):
        self.check_affinity(8, 1)

    def test_8_ranks_16_cores(self):
        self.check_affinity(8, 2)


if __name__ == '__main__':
    unittest.main()
