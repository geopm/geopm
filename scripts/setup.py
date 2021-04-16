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

import os
import sys
import shutil
import subprocess

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
    version_file = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'geopmpy/version.py')
    with open(version_file) as fid:
        exec(compile(fid.read(), version_file, 'exec'))
except IOError:
    __version__ = '0.0.0'
    __beta__ = False

    try: # Get the version from git if using standalone geopmpy
        __version__ = subprocess.check_output(['git', 'describe']).strip().decode()[1:]
        version_components = __version__.split('-')
        if len(version_components) > 1: # We are NOT on a tagged release
            tag = version_components[0]
            patches_since = version_components[1]
            git_sha = version_components[2]
            __version__ = '{}+dev{}{}'.format(tag, patches_since, git_sha)

        with open(version_file, 'w') as vf:
            vf.write("__version__ = '{}'\n".format(__version__))
            vf.write('__beta__ = {}\n'.format(__beta__))
    except OSError as ee:
        sys.stderr.write('WARNING: setting version to 0.0.0, error determining version from git - {}\n'.format(ee))

if not os.path.exists('COPYING'):
    shutil.copyfile('../COPYING', 'COPYING')
if not os.path.exists('README'):
    shutil.copyfile('../README', 'README')
if not os.path.exists('AUTHORS'):
    shutil.copyfile('../AUTHORS', 'AUTHORS')

long_description = """\
The python front end to the GEOPM runtime.  Includes scripts for
launching the runtime and postprocessing the output data."""


scripts = ["geopmlaunch"]
if __beta__:
    scripts += ["geopmplotter", "geopmconvertreport"]

classifiers = ['Development Status :: 5 - Production/Stable',
               'License :: OSI Approved :: BSD License',
               'Operating System :: POSIX :: Linux',
               'Natural Language :: English',
               'Topic :: Scientific/Engineering',
               'Topic :: Software Development :: Libraries :: Application Frameworks',
               'Topic :: System :: Distributed Computing',
               'Topic :: System :: Hardware :: Symmetric Multi-processing',
               'Topic :: System :: Power (UPS)',
               'Programming Language :: Python :: 3',
               'Programming Language :: Python :: 3.6',
]

install_requires = ['pandas>=0.23.0',
                    'natsort>=5.3.2',
                    'matplotlib>=2.2.2',
                    'cycler>=0.10.0',
                    'tables>=3.4.3',
                    'psutil>=5.4.8',
                    'cffi>=1.6.0',
                    'numpy>=1.14.3',
                    'setuptools>=39.2.0',
                    'pyyaml>=5.1.0',
                    'future>=0.17.1']

setup(name='geopmpy',
      version=__version__,
      description='GEOPM - Global Extensible Open Power Manager',
      long_description=long_description,
      url='https://geopm.github.io',
      download_url='http://download.opensuse.org/repositories/home:/cmcantalupo:/geopm/',
      license='BSD-3-Clause',
      author='Christopher Cantalupo <christopher.m.cantalupo@intel.com>, Brad Geltz <brad.geltz@intel.com>',
      packages=['geopmpy'],
      scripts=scripts,
      test_suite='test',
      classifiers=classifiers,
      install_requires=install_requires,
      python_requires='>=3.6')
