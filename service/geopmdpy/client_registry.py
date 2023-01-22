#
#  Copyright (c) 2015 - 2022, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

import gi
import os
import pwd
import grp
from gi.repository import GLib

class ClientRegistry(object):
    def __init__(self, active_sessions, destroy_session):
        self._WATCH_INTERVAL_SEC = 1
        self._active_sessions = active_sessions
        self._destroy_session = destroy_session

    def check(self, client_id):
        """Called by GLib periodically to monitor if a PID is active

        GLib queries check_client() to see if the process is still alive.

        This method gets triggered upon abnormal termination of the
        session, such as when the client process unexpectedly crashes
        or ends without closing all sessions.

        """
        if (client_id in self._active_sessions.get_clients() and
            not psutil.pid_exists(client_id)):
            self._destroy_session(client_id)
            return False
        return True

    def watch(self, client_id):
        return GLib.timeout_add_seconds(self._WATCH_INTERVAL_SEC, self.check, client_id)

    def unwatch(self, watch_id):
        GLib.source_remove(watch_id)

    def get_user(self, client_id):
        uid = os.stat(f'/proc/{client_id}/status').st_uid
        return pwd.getpwuid(uid).pw_name

    def get_groups(self, user):
        try:
            user_gid = pwd.getpwnam(user).pw_gid
            all_gid = os.getgrouplist(user, user_gid)
            user_groups = [grp.getgrgid(gid).gr_name for gid in all_gid]
        except KeyError as e:
            raise RuntimeError("Specified user '{}' does not exist.".format(user))
        return user_groups

    def validate_group(self, group):
        group = str(group)
        if group[0].isdigit():
            raise RuntimeError('Linux group name cannot begin with a digit: group = "{}"'.format(group))
        try:
            grp.getgrnam(group)
        except KeyError:
            raise RuntimeError('Linux group is not defined: group = "{}"'.format(group))

    def get_write_client(self, client_id):
        write_pid = client_pid
        client_sid = os.getsid(client_pid)
        if client_sid != client_pid and psutil.pid_exists(client_sid):
            write_pid = client_pid
        return write_pid
