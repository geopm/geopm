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

from __future__ import absolute_import

"""The GEOPM DBus API enables a client to make measurements from the
hardware platform and set hardware control parameters.  Fine grained
permissions management for both measurements (signals) and controls is
configurable by system administrators."""
_module_doc=__doc__

try:
    from docstring_parser import google
    _use_docstring_parser = True
except ImportError:
    _use_docstring_parser = False

def geopm_dbus_xml(TopoService=None, PlatformService=None):
    do_parse_docs = _use_docstring_parser
    if TopoService is None or PlatformService is None:
        do_parse_docs = False

    format_string ="""\
<node xmlns:doc="http://www.freedesktop.org/dbus/1.0/doc.dtd">
  <interface name="io.github.geopm">
    <doc:doc>
      <doc:description>
        <doc:para>{module_doc}
        </doc:para>
      </doc:description>
    </doc:doc>
    <method name="TopoGetCache">
      <arg direction="out" name="result" type="s">
        <doc:doc>
          <doc:summary>{TopoGetCache_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{TopoGetCache_short_description}
          </doc:summary>
          <doc:para>{TopoGetCache_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformGetGroupAccess">
      <arg direction="in" name="group" type="s">
        <doc:doc>
          <doc:summary>{PlatformGetGroupAccess_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="out" name="access_lists" type="(asas)">
        <doc:doc>
          <doc:summary>{PlatformGetGroupAccess_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformGetGroupAccess_short_description}
          </doc:summary>
          <doc:para>{PlatformGetGroupAccess_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformSetGroupAccess">
      <arg direction="in" name="group" type="s">
        <doc:doc>
          <doc:summary>{PlatformSetGroupAccess_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="allowed_signals" type="as">
        <doc:doc>
          <doc:summary>{PlatformSetGroupAccess_params1_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="allowed_controls" type="as">
        <doc:doc>
          <doc:summary>{PlatformSetGroupAccess_params2_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformSetGroupAccess_short_description}
          </doc:summary>
          <doc:para>{PlatformSetGroupAccess_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformGetUserAccess">
      <arg direction="out" name="access_lists" type="(asas)">
        <doc:doc>
          <doc:summary>{PlatformGetUserAccess_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformGetUserAccess_short_description}
          </doc:summary>
          <doc:para>{PlatformGetUserAccess_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformGetAllAccess">
      <arg direction="out" name="access_lists" type="(asas)">
        <doc:doc>
          <doc:summary>{PlatformGetAllAccess_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformGetAllAccess_short_description}
          </doc:summary>
          <doc:para>{PlatformGetAllAccess_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformGetSignalInfo">
      <arg direction="in" name="signal_names" type="as">
        <doc:doc>
          <doc:summary>{PlatformGetSignalInfo_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="out" name="info" type="a(ssiiii)">
        <doc:doc>
          <doc:summary>{PlatformGetSignalInfo_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformGetSignalInfo_short_description}
          </doc:summary>
          <doc:para>{PlatformGetSignalInfo_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformGetControlInfo">
      <arg direction="in" name="control_names" type="as">
        <doc:doc>
          <doc:summary>{PlatformGetControlInfo_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="out" name="info" type="a(ssi)">
        <doc:doc>
          <doc:summary>{PlatformGetControlInfo_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformGetControlInfo_short_description}
          </doc:summary>
          <doc:para>{PlatformGetControlInfo_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformLockControl">
     <doc:doc>
        <doc:description>
          <doc:summary>{PlatformLockControl_short_description}
          </doc:summary>
          <doc:para>{PlatformLockControl_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformUnlockControl">
     <doc:doc>
        <doc:description>
          <doc:summary>{PlatformUnlockControl_short_description}
          </doc:summary>
          <doc:para>{PlatformUnlockControl_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformOpenSession">
     <doc:doc>
        <doc:description>
          <doc:summary>{PlatformOpenSession_short_description}
          </doc:summary>
          <doc:para>{PlatformOpenSession_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformCloseSession">
     <doc:doc>
        <doc:description>
          <doc:summary>{PlatformCloseSession_short_description}
          </doc:summary>
          <doc:para>{PlatformCloseSession_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformCloseSessionAdmin">
      <arg direction="in" name="client_pid" type="i">
        <doc:doc>
          <doc:summary>{PlatformCloseSession_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformCloseSession_short_description}
          </doc:summary>
          <doc:para>{PlatformCloseSession_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformStartBatch">
      <arg direction="in" name="signal_config" type="a(iis)">
        <doc:doc>
          <doc:summary>{PlatformStartBatch_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="control_config" type="a(iis)">
        <doc:doc>
          <doc:summary>{PlatformStartBatch_params1_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="out" name="batch" type="(is)">
        <doc:doc>
          <doc:summary>{PlatformStartBatch_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformStartBatch_short_description}
          </doc:summary>
          <doc:para>{PlatformStartBatch_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformStopBatch">
      <arg direction="in" name="server_pid" type="i">
        <doc:doc>
          <doc:summary>{PlatformStopBatch_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformStopBatch_short_description}
          </doc:summary>
          <doc:para>{PlatformStopBatch_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformReadSignal">
      <arg direction="in" name="signal_name" type="s">
        <doc:doc>
          <doc:summary>{PlatformReadSignal_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="domain" type="i">
        <doc:doc>
          <doc:summary>{PlatformReadSignal_params1_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="domain_idx" type="i">
        <doc:doc>
          <doc:summary>{PlatformReadSignal_params2_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="out" name="sample" type="d">
        <doc:doc>
          <doc:summary>{PlatformReadSignal_returns_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformReadSignal_short_description}
          </doc:summary>
          <doc:para>{PlatformReadSignal_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
    <method name="PlatformWriteControl">
      <arg direction="in" name="control_name" type="s">
        <doc:doc>
          <doc:summary>{PlatformWriteControl_params0_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="domain" type="i">
        <doc:doc>
          <doc:summary>{PlatformWriteControl_params1_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="domain_idx" type="i">
        <doc:doc>
          <doc:summary>{PlatformWriteControl_params2_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <arg direction="in" name="setting" type="d">
        <doc:doc>
          <doc:summary>{PlatformWriteControl_params3_description}
          </doc:summary>
        </doc:doc>
      </arg>
      <doc:doc>
        <doc:description>
          <doc:summary>{PlatformWriteControl_short_description}
          </doc:summary>
          <doc:para>{PlatformWriteControl_long_description}
          </doc:para>
        </doc:description>
      </doc:doc>
    </method>
  </interface>
</node>"""

    if do_parse_docs:
        TopoGetCache = google.parse(TopoService.get_cache.__doc__)
        PlatformGetGroupAccess = google.parse(PlatformService.get_group_access.__doc__)
        PlatformSetGroupAccess = google.parse(PlatformService.set_group_access.__doc__)
        PlatformGetUserAccess = google.parse(PlatformService.get_user_access.__doc__)
        PlatformGetAllAccess = google.parse(PlatformService.get_all_access.__doc__)
        PlatformGetSignalInfo = google.parse(PlatformService.get_signal_info.__doc__)
        PlatformGetControlInfo = google.parse(PlatformService.get_control_info.__doc__)
        PlatformLockControl = google.parse(PlatformService.lock_control.__doc__)
        PlatformUnlockControl = google.parse(PlatformService.unlock_control.__doc__)
        PlatformOpenSession = google.parse(PlatformService.open_session.__doc__)
        PlatformCloseSession = google.parse(PlatformService.close_session.__doc__)
        PlatformStartBatch = google.parse(PlatformService.start_batch.__doc__)
        PlatformStopBatch = google.parse(PlatformService.stop_batch.__doc__)
        PlatformReadSignal = google.parse(PlatformService.read_signal.__doc__)
        PlatformWriteControl = google.parse(PlatformService.write_control.__doc__)
        result = format_string.format(
            module_doc=_module_doc,
            TopoGetCache_returns_description=TopoGetCache.returns.description,
            TopoGetCache_short_description=TopoGetCache.short_description,
            TopoGetCache_long_description=TopoGetCache.long_description,
            PlatformGetGroupAccess_params0_description=PlatformGetGroupAccess.params[0].description,
            PlatformGetGroupAccess_returns_description=PlatformGetGroupAccess.returns.description,
            PlatformGetGroupAccess_short_description=PlatformGetGroupAccess.short_description,
            PlatformGetGroupAccess_long_description=PlatformGetGroupAccess.long_description,
            PlatformSetGroupAccess_params0_description=PlatformSetGroupAccess.params[0].description,
            PlatformSetGroupAccess_params1_description=PlatformSetGroupAccess.params[1].description,
            PlatformSetGroupAccess_params2_description=PlatformSetGroupAccess.params[2].description,
            PlatformSetGroupAccess_short_description=PlatformGetGroupAccess.short_description,
            PlatformSetGroupAccess_long_description=PlatformGetGroupAccess.long_description,
            PlatformGetUserAccess_returns_description=PlatformGetUserAccess.returns.description,
            PlatformGetUserAccess_short_description=PlatformGetUserAccess.short_description,
            PlatformGetUserAccess_long_description=PlatformGetUserAccess.long_description,
            PlatformGetAllAccess_returns_description=PlatformGetAllAccess.returns.description,
            PlatformGetAllAccess_short_description=PlatformGetAllAccess.short_description,
            PlatformGetAllAccess_long_description=PlatformGetAllAccess.long_description,
            PlatformGetSignalInfo_params0_description=PlatformGetSignalInfo.params[0].description,
            PlatformGetSignalInfo_returns_description=PlatformGetSignalInfo.returns.description,
            PlatformGetSignalInfo_short_description=PlatformGetSignalInfo.short_description,
            PlatformGetSignalInfo_long_description=PlatformGetSignalInfo.long_description,
            PlatformGetControlInfo_params0_description=PlatformGetControlInfo.params[0].description,
            PlatformGetControlInfo_returns_description=PlatformGetControlInfo.returns.description,
            PlatformGetControlInfo_short_description=PlatformGetControlInfo.short_description,
            PlatformGetControlInfo_long_description=PlatformGetControlInfo.long_description,
            PlatformLockControl_short_description=PlatformLockControl.short_description,
            PlatformLockControl_long_description=PlatformLockControl.long_description,
            PlatformUnlockControl_short_description=PlatformUnlockControl.short_description,
            PlatformUnlockControl_long_description=PlatformUnlockControl.long_description,
            PlatformOpenSession_short_description=PlatformOpenSession.short_description,
            PlatformOpenSession_long_description=PlatformOpenSession.long_description,
            PlatformCloseSession_params0_description=PlatformCloseSession.params[0].description,
            PlatformCloseSession_short_description=PlatformCloseSession.short_description,
            PlatformCloseSession_long_description=PlatformCloseSession.long_description,
            PlatformStartBatch_params0_description=PlatformStartBatch.params[1].description,
            PlatformStartBatch_params1_description=PlatformStartBatch.params[2].description,
            PlatformStartBatch_returns_description=PlatformStartBatch.returns.description,
            PlatformStartBatch_short_description=PlatformStartBatch.short_description,
            PlatformStartBatch_long_description=PlatformStartBatch.long_description,
            PlatformStopBatch_params0_description=PlatformStopBatch.params[0].description,
            PlatformStopBatch_short_description=PlatformStopBatch.short_description,
            PlatformStopBatch_long_description=PlatformStopBatch.long_description,
            PlatformReadSignal_params0_description=PlatformReadSignal.params[0].description,
            PlatformReadSignal_params1_description=PlatformReadSignal.params[1].description,
            PlatformReadSignal_params2_description=PlatformReadSignal.params[2].description,
            PlatformReadSignal_returns_description=PlatformReadSignal.returns.description,
            PlatformReadSignal_short_description=PlatformReadSignal.short_description,
            PlatformReadSignal_long_description=PlatformReadSignal.long_description,
            PlatformWriteControl_params0_description=PlatformWriteControl.params[0].description,
            PlatformWriteControl_params1_description=PlatformWriteControl.params[1].description,
            PlatformWriteControl_params2_description=PlatformWriteControl.params[2].description,
            PlatformWriteControl_params3_description=PlatformWriteControl.params[3].description,
            PlatformWriteControl_short_description=PlatformWriteControl.short_description,
            PlatformWriteControl_long_description=PlatformWriteControl.long_description)
    else:
        result = _remove_doc(format_string)
    return result


def _remove_doc(xml_str):
    """Remove DBus XML documentation from string input

    """
    result = []
    do_add = True
    for line in xml_str.splitlines():
        if '<doc:doc>' in line:
            do_add = False
        if do_add:
            result.append(line)
        if '</doc:doc>' in line:
            do_add = True
    return '\n'.join(result)


if __name__ == '__main__':
    from . import service

    print("""<!DOCTYPE node PUBLIC
    "-//freedesktop//DTD D-BUS Object Introspection 1.0//EN"
     "http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd">
""")
    print(geopm_dbus_xml(service.TopoService, service.PlatformService))
