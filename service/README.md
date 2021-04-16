GEOPM systemd Service and Daemon
--------------------------------

The geopm service enables a client to make measurments from the
hardware platform and set hardware control parameters.  Fine grained
access management is configurable by system administrators with the
geopmaccess command line tool.

The geopmd deamon is started by the geopm systemd service and uses
dbus for communication with client processes and for administrator
configuration.

