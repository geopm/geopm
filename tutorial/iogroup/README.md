GEOPM IOGROUP TUTORIAL
======================

This directory contains a tutorial on how to extend the available
signals and controls in GEOPM by creating an IOGroup.  In general,
signals are used to read platform (hardware and OS) information while
controls are used to write to and change behavior of the platform.  An
example IOGroup can be seen in ExampleIOGroup.cpp.  The steps for
creating a new IOGroup and installing it as a plugin are listed below
and are based on this example.


0. IOGroup Interface
--------------------

IOGroups extend the IOGroup base class found in src/IOGroup.hpp.  The
pure virtual methods in this interface must be implemented by every
IOGroup.  If an IOGroup provides only signals, the methods related to
controls can have empty or degenerate implementations; the reverse is
also true if an IOGroup only provides controls.  In these cases, ensure
that is_valid_signal() or is_valid_control() returns false as appropriate, and
that signal_names() or control_names() returns an empty set.  The example plugin
used in this tutorial provides both signals and controls.

In addition to the interface methods, it can be helpful to implement
static methods assist with registration; in this example plugin_name()
and make_plugin() are implemented.  These methods will be discussed
further in the section on registration.


1. Implementing signals
-----------------------

* signal_names():

  ExampleIOGroup provides signals based on information in /proc/stat.
  "EXAMPLE::USER_TIME" represents the CPU time spent in user mode.
  "EXAMPLE::NICE_TIME" represents the CPU time spent in user mode with
  low priority.  "EXAMPLE::SYSTEM_TIME" represents the CPU time spent
  in system mode.  "EXAMPLE::IDLE_TIME" represents the CPU idle time.
  This IOGroup also provides aliases to these signals for convenience:
  "USER_TIME", "NICE_TIME", "SYSTEM_TIME", and "IDLE_TIME"
  respectively.  All four names and their aliases are returned as
  signal names.

* is_valid_signal():

  Returns true if the name provided is one from the set returned by
  signal_names().

* signal_domain_type():

  For this example, the domain of both signals is the entire board
  (M_DOMAIN_BOARD).  If the signal name is not one of the supported
  signals, it returns M_DOMAIN_INVALID.

* push_signal():

  This method does some error checking of the inputs, then sets a
  flag to indicate that the corresponding signal will be read by
  read_batch().  It returns a unique index for each signal that will
  be used when calling sample() to determine which value to return.

* read_batch():

  For each signal read flag set to true, parses the value from the
  output of /proc/stat and saves it in a variable to be used by
  future calls to sample().  The advantage of using read_batch() and
  sample() in this way is that /proc/stat only needs to be read into
  memory once to provide values for all the signals.  It also
  provides a way for the GEOPM Controller to ensure that signals
  from every IOGroup will be read at roughly the same time.  The
  values saved here are the raw strings from the file, which will be
  converted to doubles when sample() is called.  This follows the
  general pattern of IOGroups where read_batch() is responsible for
  reading the raw bytes, and sample() does the interpretation of the
  bytes to a meaningful value.  For a more realistic example of this
  interaction, refer to the MSRIOGroup, in which read_batch() reads
  the raw bits from MSRs, and sample() converts them into SI units.

* sample():

  Converts the string value previously read by read_batch() to a number
  and returns it.

* read_signal():

  Provides a value for the signal immediately by parsing /proc/stat
  (as done in read_batch()) and returning the value.  This method
  does not update the stored value for the signal because
  read_signal() should not affect future calls to sample().

* signal_description():

  Returns a string description of the given signal name.  This method
  can be used by helper applications (e.g. geopmread) to give users
  more detail about what a signal represents.


2. Implementing controls
------------------------

* control_names():

  ExampleIOGroup provides two controls: "EXAMPLE::STDOUT" and its
  alias "STDOUT" print a number to standard output.
  "EXAMPLE::STDERR" and its alias "STDERR" print a number to
  standard error.  While these controls are not very useful for real
  applications, they are provided as an example of the IOGroup
  interfaces dealing with output to the system.  Under normal
  circumstances, printing values within GEOPM should be done through
  the reporting and tracing features, not through custom controls.

* is_valid_control():

  Returns true if the name provided is one from the set returned by
  control_names().

* control_domain_type():

  For this example, the domain of the controls is the entire board
  (M_DOMAIN_BOARD).  If the control name is not one of the supported
  controls, it returns M_DOMAIN_INVALID.

* push_control():

  This method does some error checking of the inputs, then sets a
  flag to indicate that the specified control will be written during
  write_batch().

* adjust():

  The value passed into adjust is saved to be printed by a future
  call to write_batch().  Nothing will be printed at the time of the
  adjust() call.  Similarly to read_batch() and sample(), adjust()
  is responsible for the conversion from meaningful value to raw
  bytes while write_batch() handles the raw I/O.  In this example,
  the value passed into adjust() is saved as a string.  When
  write_batch() is called, it does not need to do further
  double-to-string conversion to print the value.

* write_batch():

  If the STDOUT control has been pushed, it prints the latest value
  to standard output.  If STDERR has been pushed, it prints the
  latest value to standard error.

* control_description():

  Returns a string description of the given control name.  This method
  can be used by helper applications (e.g. geopmwrite) to give users
  more detail about how to use a control.


3. Set up registration on plugin load
-------------------------------------

In order to be visible to PlatformIO and used by Agents, IOGroups must
be registered with the IOGroup factory.  The factory is a singleton
and can be accessed with the iogroup_factory() method.  The
register_plugin() method of the factory takes two arguments: the name
of the plugin ("example") and a pointer to a function that returns a
unique_ptr to a new object of the plugin type.  In this example,
ExampleIOGroup::plugin_name() provides the first argument, and
ExampleIOGroup::make_plugin is used as the second.

ExampleIOGroup is registered at the time the plugin is loaded by GEOPM
in the example_iogroup_load() method at the top of the file; the
constructor attribute indicates that this method will run at plugin
load time.  GEOPM will automatically try to load any plugins it finds
in the plugin path (discussed in the man page for geopm(7) under the
description of GEOPM_PLUGIN).  Do not link any of the GEOPM
libraries into the plugin shared object; this will cause a circular
link dependency.

The Controller will make an effort to save and restore all of the values
exposed by an IOGroup that has implemented the save_control() and
restore_control() methods.  The only built-in IOGroup with this functionality
is the MSRIOGroup.

The PlatformIO::save_control() method is implemented so that one must register
all desired IOGroups prior to the invocation of PlatformIO::save_control().
Once PlatformIO::save_control() has been called, an error is raised if
additional IOGroups are registered.  If an IOGroup plugin is loaded by a
mechanism other than using the GEOPM_PLUGIN, care must be taken to ensure
that all IOGroup plugins are loaded prior to calling Agent::init() since
PlatformIO::save_control() is called prior to the Agent::init() method.

4. Build and install
--------------------

The ExampleIOGroup plugin is built by running tutorial_build_gnu.sh or
tutorial_build_intel.sh.  The plugin will be loaded with the
geopmpolicy library if it is found in a directory in
GEOPM_PLUGIN.  Note that to be recognized as an iogroup plugin, the
filename must begin with "libgeopmiogroup\_" and end in
".so.0.0.0".  Add the current directory (containing the
.so file) to GEOPM_PLUGIN as follows:

    $ export GEOPM_PLUGIN=$PWD

An alternative is to install the plugin by copying the .so file into
the GEOPM install directory, <GEOPM_INSTALL_DIR>/lib/geopm.


5. Run with geopmread and geopmwrite
------------------------------------

Running `geopmread` with no arguments will print out a list of all
signals on the platform.  The new signals from ExampleIOGroup should
now appear in this list.  The values can be read using the signal name
as shown:

    $ geopmread USER_TIME board 0
    4932648

Running `geopmwrite` with no arguments will print out a list of all
controls on the platform.  The new controls from ExampleIOGroup should
appear in this list as well.  The controls can be tested using geopmwrite as shown:

    $ geopmwrite STDOUT board 0 12.34
    12.34

For an example Agent that uses these signals and controls, refer to the
Agent tutorial in $GEOPM_ROOT/tutorial/agent.
