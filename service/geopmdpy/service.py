#!/usr/bin/env python3
#
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

"""Module containing the implementations of the D-Bus interfaces
exposed by geopmd."""

import os
import pwd
from . import pio
from . import topo
from dasbus.connection import SystemMessageBus

def signal_info(name,
                description,
                domain,
                aggregation,
                string_format,
                behavior):
    # TODO: type checking
    return (name,
            description,
            domain,
            aggregation,
            string_format,
            behavior)

def control_info(name,
                 description,
                 domain):
    # TODO: type checking
    return (name,
            description,
            domain)

class PlatformService(object):
    def __init__(self, pio=pio, config_path='/etc/geopm-service'):
        self._pio = pio
        self._CONFIG_PATH = config_path
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'

    def get_group_access(self, group):
        group = self._validate(group)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_signals')
        signals = self._read_allowed(path)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_controls')
        controls = self._read_allowed(path)
        return signals, controls

    def set_group_access(self, group, allowed_signals, allowed_controls):
        group = self._validate(group)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_signals')
        self._write_allowed(path, allowed_signals)
        path = os.path.join(self._CONFIG_PATH, group, 'allowed_controls')
        self._write_allowed(path, allowed_controls)

    def get_user_access(self, user):
        all_groups = os.getgrouplist(user)
        signal_set = set()
        control_set = set()
        for group in all_groups:
            signals, controls = self.get_group_access(group)
            signal_set.update(signals)
            control_set.update(controls)
        signals = sorted(signal_set)
        controls = sorted(control_set)
        return signals, controls

    def get_all_access(self):
        return self._pio.signal_names(), self._pio.control_names()

    def get_signal_info(self, signal_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def get_control_info(self, control_names):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return infos

    def open_session(self, calling_pid, signal_names, control_names, interval,  protocol):
        raise NotImplementedError('PlatformService: Implementation incomplete')
        return loop_pid, clock_start, session_key

    def close_session(self):
        raise NotImplementedError('PlatformService: Implementation incomplete')

    def _read_allowed(self, path):
        try:
            with open(path) as fid:
                result = [line.strip() for line in fid.readlines() if line.strip()]
        except FileNotFoundError:
            result = []
        return result

    def _write_allowed(self, path, allowed):
        allowed.append('')
        with open(path, 'w') as fid:
            fid.write('\n'.join(allowed))

    def _validate_group(self, group):
        if group is None or group == '':
            group = self._DEFAULT_ACCESS
        else:
            group = str(group)
            if group[0].isdigit():
                raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
        return group

    def test(self):
        return self._pio.signal_names(), self._pio.control_names()


class TopoService(object):
    def __init__(self, topo=topo):
        self._topo = topo

    def get_cache(self):
        self._topo.create_cache()
        with open('/tmp/geopm-topo-cache') as fid:
            result = fid.read()
        return result


class GEOPMService(object):
    __dbus_xml__ = """
    <node>
        <interface name="io.github.geopm">
            <method name="TopoGetCache">
                <arg direction="out" name="result" type="s" />
            </method>
            <method name="PlatformGetGroupAccess">
                <arg direction="in" name="group" type="s" />
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformSetGroupAccess">
                <arg direction="in" name="group" type="s" />
                <arg direction="in" name="allowed_signals" type="as" />
                <arg direction="in" name="allowed_controls" type="as" />
            </method>
            <method name="PlatformGetUserAccess">
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformGetAllAccess">
                <arg direction="out" name="access_lists" type="(asas)" />
            </method>
            <method name="PlatformGetSignalInfo">
                <arg direction="in" name="signal_names" type="as" />
                <arg direction="out" name="info" type="a(ssiiii)" />
            </method>
            <method name="PlatformGetControlInfo">
                <arg direction="in" name="control_names" type="as" />
                <arg direction="out" name="info" type="a(ssi)" />
            </method>
            <method name="PlatformOpenSession">
                <arg direction="in" name="signal_names" type="as" />
                <arg direction="in" name="control_names" type="as" />
                <arg direction="in" name="interval" type="d" />
                <arg direction="in" name="protocol" type="i" />
                <arg direction="out" name="session" type="(i(xx)s)" />
            </method>
        </interface>
    </node>
    """
    def __init__(self, topo=TopoService(),
                 platform=PlatformService(),
                 pid=None, user=None):
        self._topo = topo
        self._platform = platform
        if None in (user, pid):
            bus = SystemMessageBus()
            dbus_proxy = bus.get_proxy('org.freedesktop.DBus', 'org/freedesktop/DBus')
            if pid is None:
                pid = dbus_proxy.GetConnectionUnixProcessID('io.github.geopm')
            if user is None:
                uid = dbus_proxy.GetConnectionUnixUser('io.github.geopm')
                user = pwd.getpwuid(uid).pw_name
        self._caller_pid = pid
        self._caller_user = user

    def TopoGetCache(self):
        return self._topo.get_cache()

    def PlatformGetGroupAccess(self, group):
        return self._platform.get_group_access(group)

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls):
        self._platform.set_group_access(group, allowed_signals, allowed_controls)

    def PlatformGetUserAccess(self):
        return self._platform.get_user_access(self._calling_user)

    def PlatformGetAllAccess(self):
        return self._platform.get_all_access()

    def PlatformGetSignalInfo(self, signal_names):
        return self._platform.get_signal_info(signal_names)

    def PlatformGetControlInfo(self, control_names):
        return self._platform.get_control_info(control_names)

    def PlatformOpenSession(self, signal_names, control_names, interval, protocol):
        return self._platform.open_session(self.calling_pid, signal_names, control_names, interval, protocol)
