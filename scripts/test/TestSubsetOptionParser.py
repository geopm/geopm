#!/usr/bin/env python
#
#  Copyright (c) 2015, 2016, 2017, Intel Corporation
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

import unittest
import geopm_context
import geopmpy.launcher

class TestSubsetOptionParser(unittest.TestCase):
    def test_all_param_known(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        parser.add_option('--long-form0', dest='lf0', nargs=1, type='string')
        parser.add_option('--long-form1', dest='lf1', nargs=1, type='string')
        parser.add_option('-a', dest='sfa', nargs=1, type='string')
        parser.add_option('-b', dest='sfb', nargs=1, type='string')

        opts, unparsed = parser.parse_args(['-aone', '-b' 'two', '--long-form0=three', '--long-form1', 'four'])
        self.assertEqual('one', opts.sfa)
        self.assertEqual('two', opts.sfb)
        self.assertEqual('three', opts.lf0)
        self.assertEqual('four', opts.lf1)
        self.assertEqual(0, len(unparsed))

    def test_all_param_unknown(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        opts, unparsed = parser.parse_args(['-aone', '-b', 'two', '--long-form0=three', '--long-form1', 'four'])
        self.assertEqual(['-aone', '-b', 'two', '--long-form0=three', '--long-form1', 'four'], unparsed)

    def test_some_param_known(self):
        parser = geopmpy.launcher.SubsetOptionParser()
        parser.add_option('--long-form0', dest='lf0', nargs=1, type='string')
        parser.add_option('--long-form1', dest='lf1', nargs=1, type='string')
        parser.add_option('-a', dest='sfa', nargs=1, type='string')
        parser.add_option('-b', dest='sfb', nargs=1, type='string')

        opts, unparsed = parser.parse_args(['-aone', '-b' 'two', '--long-form0=three', '-c', 'five', '--long-form2=six', '--long-form1', 'four'])
        self.assertEqual('one', opts.sfa)
        self.assertEqual('two', opts.sfb)
        self.assertEqual('three', opts.lf0)
        self.assertEqual('four', opts.lf1)
        self.assertEqual(['-c', 'five', '--long-form2=six'], unparsed)


if __name__ == '__main__':
    unittest.main()
