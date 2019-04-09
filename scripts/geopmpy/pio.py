#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

import cffi
import topo

class PIO(object):
    def __init__(self):
        pio_header_txt = """
int geopm_pio_num_signal_name(void);

int geopm_pio_signal_name(int name_idx,
                          size_t result_max,
                          char *result);

int geopm_pio_num_control_name(void);

int geopm_pio_control_name(int name_index,
                           size_t result_max,
                           char *result);

int geopm_pio_signal_domain_type(const char *signal_name);

int geopm_pio_control_domain_type(const char *control_name);

int geopm_pio_read_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx,
                          double *result);

int geopm_pio_write_control(const char *control_name,
                            int domain_type,
                            int domain_idx,
                            double setting);

int geopm_pio_push_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx);

int geopm_pio_push_control(const char *control_name,
                           int domain_type,
                           int domain_idx);

int geopm_pio_sample(int signal_idx,
                     double *result);

int geopm_pio_adjust(int control_idx,
                     double setting);

int geopm_pio_read_batch(void);

int geopm_pio_write_batch(void);

int geopm_pio_save_control(void);

int geopm_pio_restore_control(void);

int geopm_pio_signal_description(const char *signal_name,
                                 size_t description_max,
                                 char *description);

int geopm_pio_control_description(const char *control_name,
                                  size_t description_max,
                                  char *description);
"""
        self._ffi = cffi.FFI()
        self._ffi.cdef(pio_header_txt)
        self._dl = self._ffi.dlopen('libgeopmpolicy.so')
        self._topo = topo.Topo()

    def signal_names(self):
        """returns a list of all signal names"""
        result = []
        num_signal = self._dl.geopm_pio_num_signal_name()
        name_max = 1024
        signal_name_cstr = self._ffi.new("char[]", name_max)
        for signal_idx in range(num_signal):
            self._dl.geopm_pio_signal_name(signal_idx, name_max, signal_name_cstr)
            result.append(self._ffi.string(signal_name_cstr))
        return result

    def control_names(self):
        """returns a list of all control names"""
        result = []
        num_control = self._dl.geopm_pio_num_control_name()
        name_max = 1024
        control_name_cstr = self._ffi.new("char[]", name_max)
        for control_idx in range(num_control):
            self._dl.geopm_pio_control_name(control_idx, name_max, control_name_cstr)
            result.append(self._ffi.string(control_name_cstr))
        return result

    def signal_domain_type(self, signal_name):
        signal_name_cstr = self._ffi.new("char[]", str(signal_name))
        return self._dl.geopm_pio_signal_domain_type(signal_name_cstr)

    def control_domain_type(self, control_name):
        control_name_cstr = self._ffi.new("char[]", str(control_name))
        return self._dl.geopm_pio_control_domain_type(control_name_cstr)

    def read_signal(self, signal_name, domain_type, domain_idx):
        result_cdbl = self._ffi.new("double*")
        signal_name_cstr = self._ffi.new("char[]", str(signal_name))
        if type(domain_type) is str:
            domain_type = self._topo.domain_type(domain_type)
        err = self._dl.geopm_pio_read_signal(signal_name_cstr, domain_type, domain_idx, result_cdbl)
        if err < 0:
            raise RuntimeError('geopm_pio_read_signal() failed')
        return result_cdbl[0]

    def write_control(self, control_name, domain_type, domain_idx, setting):
        control_name_cstr = self._ffi.new("char[]", str(control_name))
        if type(domain_type) is str:
            domain_type = self._topo.domain_type(domain_type)
        err = self._dl.geopm_pio_write_control(control_name_cstr, domain_type, domain_idx, setting)
        if err < 0:
            raise RuntimeError('geopm_pio_write_control() failed')
        
    def push_signal(self, signal_name, domain_type, domain_idx):
        raise NotImplementedError
        return 0

    def push_control(self, control_name, domain_type, domain_idx):
        raise NotImplementedError
        return 0

    def sample(self, signal_idx):
        raise NotImplementedError
        return 0.0

    def adjust(self, control_idx, setting):
        raise NotImplementedError

    def read_batch(self):
        raise NotImplementedError

    def write_batch(self):
        raise NotImplementedError

    def save_control(self):
        err = self._dl.geopm_pio_save_control()
        if err < 0:
            raise RuntimeError('geopm_pio_save_control() failed')

    def restore_control(self):
        err = self._dl.geopm_pio_restore_control()
        if err < 0:
            raise RuntimeError('geopm_pio_restore_control() failed')

    def signal_description(self, signal_name):
        raise NotImplementedError
        return ''

    def control_description(self, control_name):
        raise NotImplementedError
        return ''


if __name__ == "__main__":
    import sys
    import topo

    pio = PIO()
    topo = topo.Topo()
    pio.save_control()
    try:
        time_domain_name = topo.domain_name(pio.signal_domain_type("TIME"))
        if time_domain_name != 'cpu':
            raise RuntimeError('time_domain_name = {}'.format(time_domain_name))

        sys.stdout.write('SIGNAL_NAMES:\n')
        for name in pio.signal_names():
            sys.stdout.write('    {}\n'.format(name))
        sys.stdout.write('CONTROL_NAMES:\n')
        for name in pio.control_names():
            sys.stdout.write('    {}\n'.format(name))

        time = pio.read_signal('TIME', 'cpu', 0)
        sys.stdout.write('time = {}\n'.format(time))

        try:
            power = pio.read_signal('POWER_PACKAGE', 'cpu', 0)
            sys.stdout.write('power = {}\n'.format(power))
        except RuntimeError:
            sys.stdout.write('failed to read package power\n')

        try:
            pio.write_control('FREQUENCY', 'package', 0, 1.0e9)
            sys.stdout.write('set CPU frequency for package 0 to 1 GHz\n'.format(power))
        except RuntimeError:
            sys.stdout.write('failed to write cpu frequency\n')
    except:
        pass
    pio.restore_control()
