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

from __future__ import absolute_import

import os
import sys
import unittest
import subprocess

sys.path.append(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


class TestIntegrationInstalledHeaders(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls._name = 'test_installed_headers'
        cls._prog_cpp = 'test_installed_headers.cpp'
        cls._install_dir = cls.find_geopm_install()
        cls._include_dir = os.path.join(cls._install_dir, 'include')
        cls._lib_dir = os.path.join(cls._install_dir, 'lib')
        cls._headers = cls.list_cpp_headers(cls._include_dir)

        cls._verbose = False
        if '-v' in sys.argv or '--verbose' in sys.argv:
            cls._verbose = True
        cls._test_prog = "./test_prog"  # todo: handle running outside this dir

    @classmethod
    def tearDownClass(cls):
        try:
            os.remove(cls._test_prog)
        except:
            pass

    @staticmethod
    def find_geopm_install():
        path = os.path.join(
               os.path.dirname(
                os.path.dirname(
                 os.path.realpath(__file__))),
               'config.log')
        install_path = None
        with open(path) as fid:
            for line in fid.readlines():
                if line.startswith("prefix='"):
                    install_path = line.strip().split("'")[1]
        if not install_path:
            raise RuntimeError("No prefix found in config.log")
        return install_path

    @staticmethod
    def list_cpp_headers(include_dir):
        headers = [hpp for hpp in os.listdir(os.path.join(include_dir, 'geopm'))
                   if hpp.endswith('hpp')]
        headers.remove('json11.hpp')
        return headers

    def _check_compile(self, source_file):
        failure = False
        proc = subprocess.Popen(['g++', '-std=c++11', '-I' + self._include_dir,
                                 source_file, '-lgeopmpolicy', '-L' + self._lib_dir,
                                 '-o', self._test_prog],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        with proc:
            err = proc.wait()
            if err != 0:
                failure = True
                err_out = '{}.err'.format(source_file)
                with open(err_out, 'w') as fid:
                    for line in proc.stderr.readlines():
                        fid.write(line)
                sys.stderr.write('Compilation failed.  Output written to {}\n'.format(err_out))
            elif self._verbose:
                sys.stdout.write('Pass: {} compile succeeded.\n'.format(source_file))
        return failure

    def test_standalone_includes(self):
        # test that each header can be included and built on its own
        failed_headers = []
        for head in self._headers:
            name = head.split('/')[-1].split('.')[0]
            inc_test = 'test_include_{}.cpp'.format(name)
            with open(inc_test, 'w') as fid:
                fid.write('#include "geopm/{}"\n'.format(head))
                fid.write('int main(int argc, char* argv[]) {}\n')
            failure = self._check_compile(inc_test)
            if failure:
                failed_headers.append(head)
            else:
                os.remove(inc_test)
        if len(failed_headers) > 0:
            self.fail("One or more headers is missing dependencies: {}".format(
                " ".join(failed_headers)))

    def test_example_prog_includes(self):
        # check that all headers are included in test program
        missing = []
        includes = set()
        with open(self._prog_cpp) as fid:
            for line in fid.readlines():
                if line.strip().startswith('#include') and '<' not in line:
                    includes.add(line.split('"')[1])
        for head in self._headers:
            if os.path.join('geopm', head) not in includes:
                missing.append(head)
        if len(missing) > 0:
            self.fail("The following headers are missing from test program: {}".format(
                " ".join(missing)))
        elif self._verbose:
            sys.stdout.write("Pass: all headers included in test program.\n")

    def test_example_prog_classes(self):
        # check that one object of each class is declared or used in
        # test program

        # find all classes declared in headers
        all_classes = set()
        for head in self._headers:
            with open(os.path.join(self._include_dir, 'geopm', head)) as fid:
                for line in fid.readlines():
                    line = line.strip()
                    if line.startswith('class ') and not line.endswith(';'):
                        all_classes.add(line.split()[1])

        code_lines = []
        with open(self._prog_cpp) as fid:
            for line in fid.readlines():
                if not line.strip().startswith('#include'):
                    code_lines.append(line)

        # look for occurrence of class name
        # note that this will match comments
        failure = False
        for gclass in all_classes:
            found = False
            for line in code_lines:
                if gclass in line:
                    found = True
            if not found:
                sys.stderr.write("Missing usage of class {}\n".format(gclass))
                failure = True

        if failure:
            self.fail("Test program is incomplete.")
        elif self._verbose:
            sys.stdout.write("Pass: All classes used in test program.\n")

    def test_example_prog_compile(self):
        # try to compile the test program
        failure = self._check_compile(self._prog_cpp)
        if failure:
            self.fail("Test program failed to compile")

        # try to run the test program to expose dynamic link errors
        proc = subprocess.Popen([self._test_prog],
                                stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        with proc:
            err = proc.wait()
            if err != 0:
                err_out = self._name + '.err'
                with open(err_out, 'w') as fid:
                    for line in proc.stderr.readlines():
                        fid.write(line)
                self.fail('Test program failed.  Output written to {}\n'.format(err_out))
            elif self._verbose:
                sys.stdout.write("Pass: Linking of test program succeeded.\n")


if __name__ == '__main__':
    unittest.main()
