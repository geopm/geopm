ISSUES
======

The geopm service is a new feature of GEOPM.  What exists currently is
a proof of concept / prototype, and there are many outstanding issues
that must be resolved before the geopm service is ready for release.
This TODO file serves to document known outstanding issues.  In the
long term, this file should be mapped to a series of github issues
with a common tag (geopm-service).  When the data is all stored in
issues, this file can be deleted.


Documentation
-------------

1) Some doxygen and most python doc strings are absent from the new
   code added in the geopm-service branch, these should be filled in.
2) Man pages should be created for geopm-service(8), geopmdpy(7),
   geopmd(8), geopmaccess(1), and geopmsession(1)
3) Command line help documentation for geopmaccess and geopmsession is
   lacking details.
4) Top level geopm/README.md needs to be updated to describe the
   service.
5) geopm(7) man page should be updated to describe the geopm service.
6) Error and exception handling should be made more robust so that the
   user is presented with an informative message, and unhandled
   exceptions are not propagated to command line tools.


Testing
-------

1) Unit testing of new code is almost entirely absent, some coverage
   metrics should be met both in the python code additions and in the
   new C++ code.
2) Need to create a thinner wrapper around the sd_bus C interface than
   the ServiceProxy object to enable better coverage of the
   ServiceProxy logic through mocking.
3) Integration testing of interactions with the DBus over the user bus
   should be explored to enable integration testing without root
   permissions.
4) Nightly testing should be implemented which installs the geopm
   service and then runs a battery of targeted integration tests
   against the service.
5) It is especially important to test that the service is robust in
   all user facing interfaces to bad or malicious input.


Reliability and Security
-------------------------

1) Modify systemd unit file to add "Restart=on-failure" to [Service]
   section.
   - Requires clean recovery on failure with all required state stored
     on disk.
   - The `save_control(const std::string &) method` of PlatformIO must
     be implemented to support control save state.
   - The geopmd implementation needs to be modified to call the file
     based save function.
   - The session data is being stored to disk, but these files are not
     used at startup.
   - Care must be taken to avoid problems when geopmd is killed while
     saving state.
     + Use temporary files and move into place when ready.
   - Do not release write resources until control save to disk is
     completed and all restore files are fully ready.
2) Create a robust suite of integration tests for geopmd
   failure/recovery and bad input data.


Performance
-----------

1) Currently there is no support for the batch server that is
   documented in the service/README.md.
   - There is no C++ implementation for the server/client batch
     interface over shared memory with signals.
   - The design is sketched out in the README, but many details
     about how the POSIX siginfo signals will work are unclear.
2) Optimize system overhead of running the geopm service
   - The use of the GObj event loop can be tuned, but we are using
     defaults.
   - Use of the pidfd_open() in conjunction with the
     GObj.io_add_watch() would avoid polling for each open session.
3) Create a set of integration tests that measure resource usage
   (CPU/memory) by the geopm service.
4) IOGroups should all be modified so that only signals and controls
   that are actually available are published by the object when the
   IOGroup is loaded.
   - This will enable service to provide exactly the signals and
     controls that the end user does not have access to without the
     service, but allow faster non-service path when it can be used.
   - It also avoids the case where a user can load an IOGroup, but
     fails to read or write in user space for a particular signal or
     control provided by the IOGroup.  If the signals and controls are
     published, the ServiceIOGroup will not override them, so the user
     cannot use the service to succeed with the same IOGroup created
     by geopmd as user root.
