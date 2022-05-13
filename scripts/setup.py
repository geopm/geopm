#!/usr/bin/env python3
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import os
import sys
import shutil
import subprocess # nosec

try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

if os.getcwd() != os.path.dirname(os.path.abspath(__file__)):
    sys.stderr.write('ERROR:  script must be run in the directory that contains it\n')
    sys.exit(1)

try:
    from geopmpy import __version__
    from geopmpy import __beta__
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

install_requires = ['cffi>=1.15.0',
                    'cycler>=0.11.0',
                    'natsort>=8.1.0',
                    'numpy>=1.19.5',
                    'pandas>=1.1.5',
                    'psutil>=5.9.0',
                    'pyyaml>=6.0.0',
                    'setuptools>=59.6.0',
                    'tables>=3.7.0',
                    'geopmdpy']

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
