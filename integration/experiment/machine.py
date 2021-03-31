#
#  Copyright (c) 2015 - 2021, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
        return self.signals['FREQUENCY_MIN']

    def frequency_max(self):
        return self.signals['FREQUENCY_MAX']

    def frequency_sticker(self):
        return self.signals['FREQUENCY_STICKER']

    def frequency_step(self):
        return self.signals['FREQUENCY_STEP']

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
        signal_names = ['FREQUENCY_MIN',
                        'FREQUENCY_MAX',
                        'FREQUENCY_STICKER',
                        'FREQUENCY_STEP',
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
