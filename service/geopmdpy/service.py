#!/usr/bin/env python3
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

"""Module containing the implementations of the D-Bus interfaces
exposed by geopmd."""

from . import pio
from . import topo

def signal_info(name,
                domain,
                description,
                aggregation,
                string_format,
                behavior):
    # TODO: type checking
    return (name,
            domain,
            description,
            aggregation,
            string_format,
            behavior)

def control_info(name,
                 domain,
                 description):
    # TODO: type checking
    return (name,
            domain,
            description)

class PlatformService(object):
    def init(self, pio=pio, config_path='/etc/geopm-service'):
        self._pio = pio
        self._CONFIG_PATH = config_path
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'

    def get_group_access(self, group):
        group = self._validate(group)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_signals')
        signals = self._read_allowed(path)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_controls')
        controls = self._read_allowed(path)
        return signals, controls

    def set_group_access(self, group, allowed_signals, allowed_controls):
        group = self._validate(group)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_signals')
        self._write_allowed(path, allowed_signals)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_controls')
        self._write_allowed(path, allowed_controls)

    def get_user_access(self, user):
        all_groups = os.getgrouplist(user)
        signal_set = set()
        control_set = set()
        for group in all_groups:
            signals, controls = self.get_group_access(group)
            signal_set.update(signals)
            control_set.update(controls)
        signals = sorted(signal_set)
        controls = sorted(control_set)
        return signals, controls

    def get_signal_names(self):
        return self._pio.signal_names()

    def get_control_names(self):
        return self._pio.control_names()

    def get_signal_info(self, signal_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def get_control_info(self, control_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def open_session(self, calling_pid, signal_names, control_names, interval,  protocol):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return loop_pid, clock_start, session_key

    def _read_allowed(self, path):
        try:
            with open(path) as fid:
                result = [line.strip() for line in fid.readlines() if line.strip()]
        except FileNotFoundError:
            result = []
        return result

    def _write_allowed(self, path, allowed):
        allowed.append('')
        with open(path, 'w') as fid:
            fid.write('\n'.join(allowed))

    def _validate_group(self, group):
        if group is None or group == '':
            group = self._DEFAULT_ACCESS
        else:
            group = str(group)
            if group[0].isdigit():
                raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
        return group

    def test(self):
        return self._pio.signal_names(), self._pio.control_names()


class TopoService(object):
    def __init__(self, topo=topo):
        self._topo = topo

    def get_cache(self):
        self._topo.create_cache()
        with open('/tmp/geopm-topo-cache') as fid:
            result = fid.read()
        return result
