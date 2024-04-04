from setuptools_scm import get_version
from build import ProjectBuilder
import os

script_dir = os.path.dirname(os.path.realpath(__file__))
version = get_version(f'{script_dir}/..')
with open(f'{script_dir}/geopmdpy/VERSION', 'w') as fid:
   fid.write(version)
pb = ProjectBuilder(script_dir)
pb.build('sdist', 'dist')
