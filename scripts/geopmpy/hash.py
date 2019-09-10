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

"""The hash module provides python bindings for the geopm_hash(3) C interface.
This interface provides functions to create geopm hashes, and to encode/decode
as signals.
"""
import cffi
import topo
import error
import struct

_ffi = cffi.FFI()
_ffi.cdef("""
uint64_t geopm_crc32_u64(uint64_t begin, uint64_t key);
uint64_t geopm_crc32_str(const char *key);
""")
_dl = _ffi.dlopen('libgeopmpolicy.so')

class Hash(object):
    def __init__(self, value):
        """Create a hash.

        Arguments:
        value (int): Numeric representation of the hash.
        """
        if (value | 0xffffffff) != 0xffffffff:
            raise ValueError('Hash value must be 32 bits')
        self._hash = value

    @classmethod
    def from_string_key(cls, key):
        """Create a hash from a string key.

        Arguments:
        key (str): The string to hash.
        """
        key_cstr = _ffi.new("char[]", key.encode())
        return cls(_dl.geopm_crc32_str(key_cstr))

    @classmethod
    def from_int_key(cls, begin, key):
        """Create a hash from an int key.

        Arguments:
        begin (int): The beginning value for the CRC32 hash.
        key (int): The key to be hashed.
        """
        return cls(_dl.geopm_crc32_u64(begin, key))

    @classmethod
    def from_signal(cls, signal):
        """Create a hash from its signal representation.

        Arguments:
        signal (float): The signal representation of the hash.
        """
        return cls(struct.unpack('Q', struct.pack('d', signal))[0])

    def __str__(self):
        """Return the string representation of the hash, as would be used in
        text-based configuration interfaces.
        """
        return '0x{:016x}'.format(self._hash)

    def __repr__(self):
        """Return the representation of this hash object.
        """
        return '{}({})'.format(self.__class__.__name__, str(self))

    def __int__(self):
        """Return the integer representation of the hash.
        """
        return self._hash

    def __float__(self):
        """Return the float representation of the hash.
        """
        # Reinterpret the uint64 bytes as double bytes for hashes transfered as signals.
        return struct.unpack('d', struct.pack('Q', self._hash))[0]

    def __eq__(self, other):
        return self._hash == other._hash

    def __ne__(self, other):
        return not self == other
