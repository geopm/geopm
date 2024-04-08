#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

from setuptools_scm import get_version
from build import ProjectBuilder
import os
from datetime import datetime

def setup():
    script_dir = os.path.dirname(os.path.realpath(__file__))
    version = get_version(f'{script_dir}/..')
    date = datetime.today().astimezone().strftime('%a, %d %b %Y %H:%M:%S %z')
    with open(f'{script_dir}/debian/changelog.in') as fid:
       changelog = fid.read()
    changelog = changelog.replace('@VERSION@', version).replace('@DATE@', date)
    with open(f'{script_dir}/debian/changelog', 'w') as fid:
        fid.write(changelog)
    return script_dir

def build():
    script_dir = setup()
    pb = ProjectBuilder(script_dir)
    pb.build('sdist', 'dist')

if __name__ == '__main__':
    build()
