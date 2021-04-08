#!/usr/bin/env python3

from setuptools import setup

setup(
    name='geopm-service',
    version='0.0.0',
    description='Support for the geopm service',
    url='https://geopm.github.io',
    license='BSD-3-Clause',
    data_files=[
        ('/usr/lib/systemd/system', ['system/systemd-geopm.service']),
        ('/etc/dbus-1/system.d/', ['dbus-1/system.d/io.github.geopm.conf']),
        ('/usr/bin/', ['bin/geopmd']),
    ],
    install_requires=[
        'setuptools',
        'dasbus',
        'geopmpy',
    ],
)
