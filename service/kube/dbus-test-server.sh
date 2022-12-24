#!/bin/bash

BASE_DIR=/tmp
export XDG_RUNTIME_DIR=${BASE_DIR}/XDG_RUNTIME_DIR
mkdir -p --mode=700 ${XDG_RUNTIME_DIR}
mkdir -p --mode=700 ${XDG_RUNTIME_DIR}/dbus-1

cat <<EOF > geopm-dbus.conf
<!DOCTYPE busconfig PUBLIC "-//freedesktop//DTD D-Bus Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <type>session</type>
  <keep_umask/>
  <listen>unix:path=/tmp/dbus-test-socket</listen>
  <auth>EXTERNAL</auth>
  <standard_session_servicedirs />
  <policy context="default">
    <allow user="*"/>
    <deny own="*"/>
    <allow send_type="method_call"/>
    <allow send_type="signal"/>
    <allow send_requested_reply="true" send_type="method_return"/>
    <allow send_requested_reply="true" send_type="error"/>
    <allow receive_type="method_call"/>
    <allow receive_type="method_return"/>
    <allow receive_type="error"/>
    <allow receive_type="signal"/>
    <allow send_destination="org.freedesktop.DBus"
           send_interface="org.freedesktop.DBus" />
    <allow send_destination="org.freedesktop.DBus"
           send_interface="org.freedesktop.DBus.Introspectable"/>
    <allow send_destination="org.freedesktop.DBus"
           send_interface="org.freedesktop.DBus.Properties"/>
    <allow send_destination="io.github.geopm"
           send_interface="*" />
    <deny send_destination="org.freedesktop.DBus"
          send_interface="org.freedesktop.DBus"
          send_member="UpdateActivationEnvironment"/>
    <deny send_destination="org.freedesktop.DBus"
          send_interface="org.freedesktop.DBus.Debug.Stats"/>
    <deny send_destination="org.freedesktop.DBus"
          send_interface="org.freedesktop.systemd1.Activator"/>
  </policy>
</busconfig>
EOF

strace dbus-daemon --config-file geopm-dbus.conf

