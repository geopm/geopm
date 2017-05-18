#!/usr/bin/env python
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
    execfile(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'geopm/version.py'))
except IOError:
    sys.stderr.write('WARNING:  geopm/version.py not found, setting version to 0.0.0\n')
    __version__ = '0.0.0'

if not os.path.exists('COPYING'):
    shutil.copyfile('../COPYING', 'COPYING')
if not os.path.exists('README'):
    shutil.copyfile('../README', 'README')

long_description = """\
The python front end to the GEOPM runtime.  Includes scripts for
launching the runtime and postprocessing the output data."""

scripts = ['geopmsrun',
           'geopmaprun',
           'geopmplotter']

classifiers = ['Development Status :: 2 - Pre-Alpha',
               'License :: OSI Approved :: BSD License',
               'Operating System :: POSIX :: Linux',
               'Natural Language :: English',
               'Topic :: Scientific/Engineering',
               'Topic :: Software Development :: Libraries :: Application Frameworks',
               'Topic :: System :: Distributed Computing',
               'Topic :: System :: Hardware :: Symmetric Multi-processing',
               'Topic :: System :: Power (UPS)']

install_requires = ['pandas',
                    'natsort',
                    'cycler']

setup(name='geopm',
      version=__version__,
      description='GEOPM - Global Extensible Open Power Manager',
      long_description=long_description,
      url='https://geopm.github.io/geopm',
      download_url='http://download.opensuse.org/repositories/home:/cmcantalupo:/geopm/',
      license='BSD-3-Clause',
      author='Christopher Cantalupo',
      author_email='christopher.m.cantalupo@intel.com',
      packages=['geopm'],
      scripts=scripts,
      classifiers=classifiers,
      install_requires=install_requires)
