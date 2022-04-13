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
        return self.signals['CPU_FREQUENCY_MIN']

    def frequency_max(self):
        return self.signals['CPU_FREQUENCY_MAX']

    def frequency_sticker(self):
        return self.signals['CPU_FREQUENCY_STICKER']

    def frequency_step(self):
        return self.signals['CPU_FREQUENCY_STEP']

    def power_package_min(self):
        return self.signals['POWER_PACKAGE_MIN']

    def power_package_tdp(self):
        return self.signals['POWER_PACKAGE_TDP']

    def power_package_max(self):
        return self.signals['POWER_PACKAGE_MAX']

    def num_board(self):
        return int(self.topo['board'])

    def num_package(self):
        return int(self.topo['package'])

    def num_core(self):
        return int(self.topo['core'])

    def num_cpu(self):
        return int(self.topo['cpu'])

    def num_board_memory(self):
        return int(self.topo['board_memory'])

    def num_package_memory(self):
        return int(self.topo['package_memory'])

    def num_board_nic(self):
        return int(self.topo['board_nic'])

    def num_package_nic(self):
        return int(self.topo['package_nic'])

    def num_board_accelerator(self):
        return int(self.topo['board_accelerator'])

    def num_package_accelerator(self):
        return int(self.topo['package_accelerator'])

    def total_node_memory_bytes(self):
        return float(self.meminfo['MemTotal'])

    def _query(self):
        self.signals = {}
        signal_names = ['CPU_FREQUENCY_MIN',
                        'CPU_FREQUENCY_MAX',
                        'CPU_FREQUENCY_STICKER',
                        'CPU_FREQUENCY_STEP',
                        'POWER_PACKAGE_MIN',
                        'POWER_PACKAGE_TDP',
                        'POWER_PACKAGE_MAX']
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
