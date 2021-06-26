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
import grp
import json
import shutil
import psutil
import gi
from gi.repository import GLib

from . import pio
from . import topo
from dasbus.connection import SystemMessageBus
try:
    from dasbus.server.interface import accepts_additional_arguments
except ImportError as ee:
    err_msg = '''dasbus version greater than 1.5 required:
    https://github.com/rhinstaller/dasbus/pull/57'''
    raise ImportError(err_msg) from ee

class PlatformService(object):
    def __init__(self):
        """PlatformService constructor
        """
        self._pio = pio
        self._CONFIG_PATH = '/etc/geopm-service'
        self._VAR_PATH = '/var/run/geopm-service'
        self._ALL_GROUPS = [gg.gr_name for gg in grp.getgrall()]
        self._DEFAULT_ACCESS = '0.DEFAULT_ACCESS'
        self._SAVE_DIR = 'SAVE_FILES'
        self._WATCH_INTERVAL_MSEC = 1000
        self._write_pid = None
        self._sessions = dict()

    def get_group_access(self, group):
        """Get the signals and controls in the allowed lists.

        Provides the default allowed lists or the allowed lists for a
        particular Unix group.  These lists correspond to values
        stored in the GEOPM service configuration files.  The
        configuration files are read with each call to this method.
        If no files exist that match the query, empty lists are
        returned.  Empty lines and lines that begin with the '#'
        character are ignored.  If the group name is not valid on the
        system a RuntimeError is raised.

        Args:
            group (str): Unix group name to query. The default allowed
                lists are returned if group is the empty string.

        Returns:
            list(str), list(str): Signal and control allowed lists

        """
        group = self._validate_group(group)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        if os.path.isdir(group_dir):
            path = os.path.join(group_dir, 'allowed_signals')
            signals = self._read_allowed(path)
            path = os.path.join(group_dir, 'allowed_controls')
            controls = self._read_allowed(path)
        else:
            signals = []
            controls = []
        return signals, controls

    def set_group_access(self, group, allowed_signals, allowed_controls):
        """Set the signals and controls in the allowed lists.

        Writes the configuration files that control the default
        allowed lists or the allowed lists for a particular Unix
        group.  These lists restrict user access to signals or
        controls provided by the service.  If the user specifies a
        list that contains signals or controls that are not currently
        supported, the request will raise a RuntimeError without
        modifying any configuration files.  If the group name is not
        valid on the system a RuntimeError is also raised.

        Args:
            group (str): Unix group name to query. The default allowed
                lists are written if group is the empty string.

            allowed_signals (list(str)): Signal names that are allowed

            allowed_controls (list(str)): Control names that are allowed

        """
        group = self._validate_group(group)
        self._validate_signals(allowed_signals)
        self._validate_controls(allowed_controls)
        group_dir = os.path.join(self._CONFIG_PATH, group)
        os.makedirs(group_dir, exist_ok=True)
        path = os.path.join(group_dir, 'allowed_signals')
        self._write_allowed(path, allowed_signals)
        path = os.path.join(group_dir, 'allowed_controls')
        self._write_allowed(path, allowed_controls)

    def get_user_access(self, user):
        """When a user requests a signal or control they are restricted to the
        superset of the default allowed list and the allowed list for
        all Unix groups that the user belongs to.

        """
        if user == 'root':
            return self.get_all_access()
        user_groups = self._get_user_groups(user)
        user_groups.append('') # Default access list
        signal_set = set()
        control_set = set()
        for group in user_groups:
            signals, controls = self.get_group_access(group)
            signal_set.update(signals)
            control_set.update(controls)
        signals = sorted(signal_set)
        controls = sorted(control_set)
        return signals, controls

    def get_all_access(self):
        return self._pio.signal_names(), self._pio.control_names()

    def get_signal_info(self, signal_names):
        result = []
        for name in signal_names:
            description = self._pio.signal_description(name)
            domain_type = self._pio.signal_domain_type(name)
            aggregation, format_type, behavior = self._pio.signal_info(name)
            result.append((name, description, domain_type, aggregation, format_type, behavior))
        return result

    def get_control_info(self, control_names):
        result = []
        for name in control_names:
            description = self._pio.control_description(name)
            domain_type = self._pio.control_domain_type(name)
            result.append((name, description, domain_type))
        return result

    def lock_control(self):
        raise NotImplementedError('PlatformService: Implementation incomplete')

    def unlock_control(self):
        raise NotImplementedError('PlatformService: Implementation incomplete')

    def open_session(self, user, client_pid):
        """Method that creates a new client session"""
        signals, controls = self.get_user_access(user)
        os.makedirs(self._VAR_PATH, exist_ok=True)
        session_file = self._get_session_file(client_pid)
        if os.path.isfile(session_file):
            raise RuntimeError('Session file for connecting process already exists: {}'.format(session_file))
        watch_id = self._watch_client(client_pid)
        session_data = {'client_pid': client_pid,
                        'mode': 'r',
                        'signals': signals,
                        'controls': controls,
                        'watch_id': watch_id}
        self._sessions[client_pid] = session_data
        with open(session_file, 'w') as fid:
            json.dump(session_data, fid)

    def close_session(self, client_pid):
        session = self._get_session(client_pid, 'PlatformCloseSession')
        if client_pid == self._write_pid:
            save_dir = os.path.join(self._VAR_PATH, self._SAVE_DIR)
            self._pio.restore_control()
            shutil.rmtree(save_dir)
            self._write_pid = None
        GLib.source_remove(session['watch_id'])
        self._sessions.pop(client_pid)
        session_file = self._get_session_file(client_pid)
        os.remove(session_file)

    def start_batch(self, client_pid, signal_config, control_config):
        session = self._get_session(client_pid, 'PlatformStartBatch')
        sig_req = {cc[2] for cc in signal_config}
        cont_req = {cc[2] for cc in control_config}
        if not sig_req.issubset(session['signals']):
            raise RuntimeError('Requested signals that are not in allowed list')
        elif not cont_req.issubset(session['controls']):
            raise RuntimeError('Requested controls that are not in allowed list')
        if len(control_config) != 0:
            self._write_mode(client_pid)
        return self._pio.start_batch_server(client_pid, signal_config, control_config)

    def stop_batch(self, client_pid, server_pid):
        self._get_session(client_pid, 'StopBatch')
        self._pio.stop_batch_server(server_pid)

    def read_signal(self, client_pid, signal_name, domain, domain_idx):
        session = self._get_session(client_pid, 'PlatformReadSignal')
        if not signal_name in session['signals']:
            raise RuntimeError('Requested signal that is not in allowed list')
        return self._pio.read_signal(signal_name, domain, domain_idx)

    def write_control(self, client_pid, control_name, domain, domain_idx, setting):
        session = self._get_session(client_pid, 'PlatformWriteControl')
        if not control_name in session['controls']:
            raise RuntimeError('Requested control that is not in allowed list')
        self._write_mode(client_pid)
        self._pio.write_control(control_name, domain, domain_idx, setting)

    def _read_allowed(self, path):
        try:
            with open(path) as fid:
                result = [line.strip() for line in fid.readlines()
                          if line.strip() and not line.strip().startswith('#')]
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
            if group not in self._ALL_GROUPS:
                raise RuntimeError('Linux group is not defined: group = "{}"'.format(group))
        return group

    def _validate_signals(self, signals):
        signals = set(signals)
        all_signals = self._pio.signal_names()
        if not signals.issubset(all_signals):
            unmatched = signals.difference(all_signals)
            err_msg = 'The service does not support any signals that match: "{}"'.format('", "'.join(unmatched))
            raise RuntimeError(err_msg)

    def _validate_controls(self, controls):
        controls = set(controls)
        all_controls = self._pio.control_names()
        if not controls.issubset(all_controls):
            unmatched = controls.difference(all_controls)
            err_msg = 'The service does not support any controls that match: "{}"'.format('", "'.join(unmatched))
            raise RuntimeError(err_msg)

    def _get_user_groups(self, user):
        user_gid = pwd.getpwnam(user).pw_gid
        all_gid = os.getgrouplist(user, user_gid)
        return [grp.getgrgid(gid).gr_name for gid in all_gid]

    def _get_session(self, client_pid, operation):
        try:
            session = self._sessions[client_pid]
        except KeyError:
            raise RuntimeError('Operation {} not allowed without an open session'.format(operation))
        return session

    def _get_session_file(self, client_pid):
        return os.path.join(self._VAR_PATH, 'session-{}.json'.format(client_pid))

    def _write_mode(self, client_pid):
        if self._sessions[client_pid]['mode'] != 'rw':
            if self._write_pid is not None:
                raise RuntimeError('The geopm service already has a connected "rw" mode client')
            session_file = self._get_session_file(client_pid)
            self._sessions[client_pid]['mode'] = 'rw'
            with open(session_file, 'w') as fid:
                json.dump(self._sessions[client_pid], fid)
            self._write_pid = client_pid
            save_dir = os.path.join(self._VAR_PATH, self._SAVE_DIR)
            os.makedirs(save_dir)
            # TODO: Will need to save to disk in order to support
            # daemon restart
            self._pio.save_control()

    def _watch_client(self, client_pid):
        return GLib.timeout_add(self._WATCH_INTERVAL_MSEC, self._check_client, client_pid)

    def _check_client(self, client_pid):
        if client_pid in self._sessions and not psutil.pid_exists(client_pid):
            self.close_session(client_pid)
            return False
        return True

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
            <method name="PlatformLockControl" />
            <method name="PlatformUnlockControl" />
            <method name="PlatformOpenSession" />
            <method name="PlatformCloseSession" />
            <method name="PlatformCloseSessionAdmin">
                <arg direction="in" name="client_pid" type="i" />
            </method>
            <method name="PlatformStartBatch">
                <arg direction="in" name="signal_config" type="a(iis)" />
                <arg direction="in" name="control_config" type="a(iis)" />
                <arg direction="out" name="batch" type="(is)" />
            </method>
            <method name="PlatformStopBatch">
                <arg direction="in" name="server_pid" type="i" />
            </method>
            <method name="PlatformReadSignal">
                <arg direction="in" name="signal_name" type="s" />
                <arg direction="in" name="domain" type="i" />
                <arg direction="in" name="domain_idx" type="i" />
                <arg direction="out" name="sample" type="d" />
            </method>
            <method name="PlatformWriteControl">
                <arg direction="in" name="control_name" type="s" />
                <arg direction="in" name="domain" type="i" />
                <arg direction="in" name="domain_idx" type="i" />
                <arg direction="in" name="setting" type="d" />
            </method>
        </interface>
    </node>
    """
    def __init__(self, topo=TopoService(),
                 platform=PlatformService()):
        self._topo = topo
        self._platform = platform
        self._write_pid = None
        self._dbus_proxy = SystemMessageBus().get_proxy('org.freedesktop.DBus',
                                                        '/org/freedesktop/DBus')

    def TopoGetCache(self):
        return self._topo.get_cache()

    def PlatformGetGroupAccess(self, group):
        return self._platform.get_group_access(group)

    def PlatformSetGroupAccess(self, group, allowed_signals, allowed_controls):
        self._platform.set_group_access(group, allowed_signals, allowed_controls)

    @accepts_additional_arguments
    def PlatformGetUserAccess(self, **call_info):
        return self._platform.get_user_access(self._get_user(**call_info))

    def PlatformGetAllAccess(self):
        return self._platform.get_all_access()

    def PlatformGetSignalInfo(self, signal_names):
        return self._platform.get_signal_info(signal_names)

    def PlatformGetControlInfo(self, control_names):
        return self._platform.get_control_info(control_names)

    def PlatformLockControl(self):
        self._platform.lock_control()

    def PlatformUnlockControl(self):
        self._platform.unlock_control()

    @accepts_additional_arguments
    def PlatformOpenSession(self, **call_info):
        self._platform.open_session(self._get_user(**call_info), self._get_pid(**call_info))

    @accepts_additional_arguments
    def PlatformCloseSession(self, **call_info):
        self._platform.close_session(self._get_pid(**call_info))

    def PlatformCloseSessionAdmin(self, client_pid):
        self._platform.close_session(client_pid)

    @accepts_additional_arguments
    def PlatformStartBatch(self, signal_config, control_config, **call_info):
        return self._platform.start_batch(self._get_pid(**call_info), signal_config, control_config)

    @accepts_additional_arguments
    def PlatformStopBatch(self, server_pid, **call_info):
        self._platform.stop_batch(self._get_pid(**call_info), server_pid)

    @accepts_additional_arguments
    def PlatformReadSignal(self, signal_name, domain, domain_idx, **call_info):
        return self._platform.read_signal(self._get_pid(**call_info), signal_name, domain, domain_idx)

    @accepts_additional_arguments
    def PlatformWriteControl(self, control_name, domain, domain_idx, setting, **call_info):
        self._platform.write_control(self._get_pid(**call_info), control_name, domain, domain_idx, setting)

    def _get_user(self, call_info):
        unique_name = call_info['sender']
        uid = self._dbus_proxy.GetConnectionUnixUser(unique_name)
        return pwd.getpwuid(uid).pw_name

    def _get_pid(self, call_info):
        unique_name = call_info['sender']
        return self._dbus_proxy.GetConnectionUnixProcessID(unique_name)
