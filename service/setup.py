#!/usr/bin/env python3

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

from setuptools import setup

import os
import sys
import shutil

from setuptools import setup

if os.getcwd() != os.path.dirname(os.path.abspath(__file__)):
    sys.stderr.write('ERROR:  script must be run in the directory that contains it\n')
    sys.exit(1)

try:
    # use excfile rather than import so that setup.py can be executed
    # on a system missing dependencies required to import geopmpy.
    version_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'geopmdpy/version.py')
    with open(version_file) as fid:
        exec(compile(fid.read(), version_file, 'exec'))
except IOError:
    sys.stderr.write('WARNING:  geopmpy/version.py not found, setting version to 0.0.0\n')
    __version__ = '0.0.0'
    __beta__ = False

long_description = """\
The python implementation for the GEOPM daemon"""


scripts = ['geopmd', 'geopmaccess', 'geopmsession']

classifiers = ['Development Status :: 5 - Production/Stable',
               'License :: OSI Approved :: BSD License',
               'Operating System :: POSIX :: Linux',
               'Natural Language :: English',
               'Topic :: Scientific/Engineering',
               'Topic :: Software Development :: Libraries :: Application Frameworks',
               'Topic :: System :: Hardware :: Symmetric Multi-processing',
               'Topic :: System :: Power (UPS)',
               'Programming Language :: Python :: 3',
               'Programming Language :: Python :: 3.6',
]

install_requires = ['cffi>=1.6.0',
                    'setuptools>=39.2.0',
                    'future>=0.17.1',
                    'psutil>=5.6.2',
                    'dasbus>=1.5.0',
                    'jsonschema>=2.6.0']

setup(name='geopmdpy',
      version=__version__,
      description='GEOPM - Global Extensible Open Power Manager Daemon',
      long_description=long_description,
      url='https://geopm.github.io',
      download_url='https://github.com/geopm/geopm/releases',
      license='BSD-3-Clause',
      author='Christopher Cantalupo <christopher.m.cantalupo@intel.com>',
      packages=['geopmdpy'],
      scripts=scripts,
      classifiers=classifiers,
      install_requires=install_requires,
      python_requires='>=3.6')
