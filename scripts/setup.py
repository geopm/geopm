#!/usr/bin/env python
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

import os
import sys
import shutil
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

if os.getcwd() != os.path.dirname(os.path.abspath(__file__)):
    sys.stderr.write('ERROR:  script must be run in the directory that contains it\n')
    exit(1)

try:
    # use excfile rather than import so that setup.py can be executed
    # on a system missing dependencies required to import geopmpy.
    execfile(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'geopmpy/version.py'))
except IOError:
    sys.stderr.write('WARNING:  geopm/version.py not found, setting version to 0.0.0\n')
    __version__ = '0.0.0'

if not os.path.exists('COPYING'):
    shutil.copyfile('../COPYING', 'COPYING')
if not os.path.exists('README'):
    shutil.copyfile('../README', 'README')
if not os.path.exists('AUTHORS'):
    shutil.copyfile('../AUTHORS', 'AUTHORS')

long_description = """\
The python front end to the GEOPM runtime.  Includes scripts for
launching the runtime and postprocessing the output data."""

scripts = ['geopmsrun',
           'geopmaprun',
           'geopmplotter']

classifiers = ['Development Status :: 3 - Alpha',
               'License :: OSI Approved :: BSD License',
               'Operating System :: POSIX :: Linux',
               'Natural Language :: English',
               'Topic :: Scientific/Engineering',
               'Topic :: Software Development :: Libraries :: Application Frameworks',
               'Topic :: System :: Distributed Computing',
               'Topic :: System :: Hardware :: Symmetric Multi-processing',
               'Topic :: System :: Power (UPS)']

install_requires = ['pandas>=0.19.2',
                    'natsort',
                    'matplotlib',
                    'cycler']

setup(name='geopmpy',
      version=__version__,
      description='GEOPM - Global Extensible Open Power Manager',
      long_description=long_description,
      url='https://geopm.github.io/geopm',
      download_url='http://download.opensuse.org/repositories/home:/cmcantalupo:/geopm/',
      license='BSD-3-Clause',
      author='Christopher Cantalupo <christopher.m.cantalupo@intel.com>, Brad Geltz <brad.geltz@intel.com>',
      packages=['geopmpy'],
      scripts=scripts,
      test_suite='test',
      classifiers=classifiers,
      install_requires=install_requires,
      python_requires='>=2.7,<3')
