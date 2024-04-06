#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from setuptools_scm import get_version
from build import ProjectBuilder
import os
from datetime import datetime

script_dir = os.path.dirname(os.path.realpath(__file__))
version = get_version(f'{script_dir}/..')
with open(f'{script_dir}/geopmpy/VERSION', 'w') as fid:
   fid.write(version)
with open(f'{script_dir}/debian/changelog.in') as fid:
   changelog = fid.read()
with open(f'{script_dir}/debian/changelog', 'w') as fid:
   date = datetime.today().astimezone().strftime('%a, %d %b %Y %H:%M:%S %z')
   changelog = changelog.replace('@VERSION@', version).replace('@DATE@', date)
   fid.write(changelog)
pb = ProjectBuilder(script_dir)
pb.build('sdist', 'dist')
