#!/usr/bin/env python3

#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
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
    from geopmdpy import __version__
except IOError:
    sys.stderr.write('WARNING:  geopmpy/version.py not found, setting version to 0.0.0\n')
    __version__ = '0.0.0'

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

install_requires = ['cffi>=1.15.0',
                    'setuptools>=59.6.0',
                    'psutil>=5.9.0',
                    'dasbus>=1.6.0',
                    'jsonschema>=3.2.0']

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
