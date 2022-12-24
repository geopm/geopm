USE AT YOUR OWN RISK, QoS (CONTROL SAVE/RESTORE) NOT IMPLEMENTED
================================================================

Trying to get GEOPM service up and running in kubernetes.  This uses
an experimental build of the geopm-service.  In this build some
features to allow anonymous access through the service have been
added.  It is important to note that because PID origin is unknown in
this implementation (no use of systemd dbus service), there is no
save/restore mechanism for controls.  This feature will be added back
later.

The GEOPM service will filter requests based on the default access
list as is defined in the geopm-server.sh script.  Currently that
access list contains only the TIME signal.  However, if controls were
also enabled, they may be modified without being restored when process
session ends.  The authentication for the save/restore feature is
managed by systemd in the non-containerized solution.

To create the client and server containers execute the
`geopm-create.sh` script in this working directory.

December 24, 2022
Christopher Cantalupo
