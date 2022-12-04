#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import json
import os
import sys
import glob

from . import util


class Machine:
    def __init__(self, outdir='.'):
        self.signals = {}
        self.path = os.path.join(outdir, 'machine.json')

    def load(self):
        try:
            with open(self.path) as fid:
                data = json.load(fid)
                try:
                    self.signals = data['signals']
                    self.topo = data['topo']
                    try:
                        self.meminfo = data['meminfo']
                    except:
                        self.meminfo = {}
                except:
                    self.signals = data
                    self.topo = {}
                    self.meminfo = {}
        except:
            raise RuntimeError("<geopm> Unable to open machine config {}.".format(self.path))

    def save(self):
        glob_pattern = os.path.join(os.path.dirname(self.path), '*.report')
        if os.path.exists(self.path) and glob.glob(glob_pattern):
            raise RuntimeError('<geopm> Machine.save(): function called twice with same path, and reports exist in the same directory.  File exists: {}'.format(self.path))
        self._query()
        data = {
            'signals': self.signals,
            'topo': self.topo,
            'meminfo': self.meminfo
        }
        with open(self.path, 'w') as info_file:
            json.dump(data, info_file)

    def frequency_min(self):
        return self.signals['CPU_FREQUENCY_MIN_AVAIL']

    def frequency_max(self):
        return self.signals['CPU_FREQUENCY_MAX_AVAIL']

    def frequency_sticker(self):
        return self.signals['CPU_FREQUENCY_STICKER']

    def frequency_step(self):
        return self.signals['CPU_FREQUENCY_STEP']

    def gpu_frequency_step(self):
        return self.signals['GPU_CORE_FREQUENCY_STEP']

    def power_package_min(self):
        return self.signals['CPU_POWER_MIN_AVAIL']

    def power_package_tdp(self):
        return self.signals['CPU_POWER_LIMIT_DEFAULT']

    def power_package_max(self):
        return self.signals['CPU_POWER_MAX_AVAIL']

    def num_board(self):
        return int(self.topo['board'])

    def num_package(self):
        return int(self.topo['package'])

    def num_core(self):
        return int(self.topo['core'])

    def num_cpu(self):
        return int(self.topo['cpu'])

    def num_memory(self):
        return int(self.topo['memory'])

    def num_package_integrated_memory(self):
        return int(self.topo['package_integrated_memory'])

    def num_nic(self):
        return int(self.topo['nic'])

    def num_package_integrated_nic(self):
        return int(self.topo['package_integrated_nic'])

    def num_gpu(self):
        return int(self.topo['gpu'])

    def num_package_integrated_gpu(self):
        return int(self.topo['package_integrated_gpu'])

    def total_node_memory_bytes(self):
        return float(self.meminfo['MemTotal'])

    def _query(self):
        self.signals = {}
        signal_names = ['CPU_FREQUENCY_MIN_AVAIL',
                        'CPU_FREQUENCY_MAX_AVAIL',
                        'CPU_FREQUENCY_STICKER',
                        'CPU_FREQUENCY_STEP',
                        'GPU_CORE_FREQUENCY_STEP',
                        'CPU_POWER_MIN_AVAIL',
                        'CPU_POWER_LIMIT_DEFAULT',
                        'CPU_POWER_MAX_AVAIL']
        for sn in signal_names:
            self.signals[sn] = util.geopmread('{} board 0'.format(sn))

        self.topo = util.geopmread_domain()
        self.meminfo = util.get_node_memory_info()


def init_output_dir(output_dir):
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)
    elif not os.path.isdir(output_dir):
        raise RuntimeError('Requested output directory is a file: {}'.format(output_dir))
    mm = Machine(output_dir)
    mm.save()
    return mm


def get_machine(output_dir):
    mm = Machine(output_dir)
    mm.load()
    return mm

def try_machine(output_dir, msg_tag=None):
    mm = Machine()
    try:
        mm.load()
        err_msg = ['Warning']
        if msg_tag:
           err_msg.append(msg_tag)
        err_msg.append('using existing file "machine.json", delete if invalid\n')
        err_msg = ': '.join(err_msg)
        sys.stderr.write(err_msg)
    except RuntimeError:
        mm.save()
    return mm
