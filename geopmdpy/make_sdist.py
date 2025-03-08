#  Copyright (c) 2015 - 2025 Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from setuptools_scm import get_version
from build import ProjectBuilder
import os
from datetime import datetime

def setup():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    package_name = os.path.basename(script_dir)
    version = get_version(f'{script_dir}/..')
    with open(f'{script_dir}/{package_name}/VERSION', 'w') as fid:
        fid.write(version)
    date = datetime.today().astimezone().strftime('%a, %d %b %Y %H:%M:%S %z')
    with open(f'{script_dir}/debian/changelog.in') as fid:
       changelog = fid.read()
    changelog = changelog.replace('@VERSION@', version).replace('@DATE@', date)
    with open(f'{script_dir}/debian/changelog', 'w') as fid:
        fid.write(changelog)
    with open(f'{script_dir}/geopmd.spec.in') as fid:
        specfile = fid.read()
    archive = f'{package_name}-{version}.tar.gz'
    specfile = specfile.replace('@VERSION@', version).replace('@ARCHIVE@', archive)
    with open(f'{script_dir}/geopmd.spec', 'w') as fid:
        fid.write(specfile)
    return script_dir

def build():
    script_dir = setup()
    pb = ProjectBuilder(script_dir)
    pb.build('sdist', 'dist')

if __name__ == '__main__':
    build()
