ISSUES
======

The GEOPM systemd service is a new feature.  What exists currently is
a proof of concept / prototype, and there are many outstanding issues
that must be resolved before the GEOPM service is ready for release.
This TODO file serves to document known outstanding issues.  In the
long term, this file should be mapped to a series of github issues
with a common tag (geopm-service).  When the data is all stored in
issues, this file can be deleted.


Documentation
-------------

1) Some doxygen and most python doc strings are absent from the new
   code added in the geopm-service branch, these should be filled in.
2) Man pages should be created for geopm-service(8), geopmdpy(8),
   geopmd(8), geopmaccess(1), and geopmsession(1)
3) Command line help documentation for geopmaccess and geopmsession is
   lacking details.
4) Top level geopm/README.md needs to be updated to describe the
   service.
5) geopm(7) man page should be updated to describe the geopm service.
6) Error and exception handling should be made more robust so that the
   user is presented with an informative message, and unhandled
   exceptions are not propagated to command line tools.
7) The text in the geopm/service/README.md should give with some
   motivations and use cases and help provide more context for the
   problem it is solving.
8) Write a system administrators guide to configuring and deploying
   the geopm service to be included in the geopm-service(8) man page.


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
     used at start-up.
   - Care must be taken to avoid problems when geopmd is killed while
     saving state.
     + Use temporary files and move into place when ready.
   - Do not release write resources until control save to disk is
     completed and all restore files are fully ready.
2) Create a robust suite of integration tests for geopmd
   failure/recovery and bad input data.
3) The IOGroups identify monotonic signals, and the service README
   describes a filtering algorithm where these are zero value on the
   first read of a client session.  This security feature has not yet
   been implemented for the io.github.geopm.PlatformReadSignal
   service, and should be part of the batch server implementation as
   well.
4) Use Linux cgroups(7) to filter signal and control requests based on
   cgroup restrictions on the client PID, in particular the "cpuset"
   should be pretty straight forward to implement.  We may also want
   to consider accounting for other restrictions such as "rdma", "io",
   or "perf_event".


Performance
-----------

1) Currently there is no support for the batch server that is
   documented in the service/README.md.
   - There is no C++ implementation for the server/client batch
     interface over shared memory with signals.
   - The design is sketched out in the README, but some details
     about how the POSIX siginfo signals will work are unclear.
2) Optimize system overhead of running the geopm service
   - The use of the GObj event loop can be tuned, but we are using
     defaults.
   - Consider using GLib.timeout_add_seconds() instead of
     GLib.timeout_add()
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

Features
--------

1) The geopmsession CSV generation is not well aligned with the GEOPM
   runtime trace generation.  It does not have to be, but the choices
   were somewhat arbitrary.
   - The output of geopmsession does not honor the signal string
     formatting specified by the IOGroup the way the GEOPM trace does,
     e.g. integers are formatted as double precision.
   - There is no header to the output identifying signal names and
     domains.
   - Commas (',') were chosen rather than pipes ('|') for the CSV
     delimiter.
2) Currently the user of geopmsession could specify the energy for
   package both packages as the following standard input

       ENERGY_PACKAGE package 0
       ENERGY_PACKAGE package 1

   It would be nice if the user could specify input as a comma
   separated list as it might appear in the the trace header:

       --signals=ENERGY_PACKAGE@package-0,ENERGY_PACKAGE@package-1

   and enable "--signals=ENERGY_PACKAGE@package" to imply: add signal
   for all packages.  This could be a command line option supported in
   place of using standard input for read sessions.
3) The dasbus interface enables automatic generation of interface xml.
   We should try to take advantage of this feature and additionally
   use it to generate the interface xml file.


Duplicated Code
---------------

1) The pio.py, topo.py error.py and version.py were copied from
   geopmpy to geopmdpy.  These are bindings to C interfaces and in the
   case of geopmdpy these bind to libgeopmd and for geopmpy these bind
   to libgeopmpolicy.
2) The service directory is constructed as an independent package,
   perhaps it would be better to have the geopm service be a
   dependency libgeopmpolicy, libgeopm and geopmpy.  Might want to
   consider changing some .so names in the process.
