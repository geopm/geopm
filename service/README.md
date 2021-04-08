GEOPM systemd Service and Daemon
--------------------------------

The geopm service enables a client to make measurments from the
hardware platform and set hardware control parameters.  Fine grained
access management is configurable by system administrators with the
geopmaccess tool.

The geopmd deamon is started by the geopm systemd service and uses
dbus for communication with client processes and for administrator
configuration.  The following dbus interfaces are enabled with the
geopm service:

- io.github.geopm.AccessList
- io.github.geopm.AccessControl
- io.github.geopm.AccessAllowed
- io.github.geopm.Loop
- io.github.geopm.SignalInfo
- io.github.geopm.ControlInfo
