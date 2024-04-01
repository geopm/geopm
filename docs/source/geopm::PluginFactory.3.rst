
geopm::PluginFactory(3) -- abstract factory for plugins
=======================================================


Namespaces
----------

The ``PluginFactory`` class template is a member of the ``namespace geopm``\ , but
the full name, ``geopm::PluginFactory<class T>``\ , has been abbreviated in this
manual.  Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::set``\ , and ``std::function``\ , to enable better rendering of
this manual.

Synopsis
--------

#include `<geopm/PluginFactory.hpp> <https://github.com/geopm/geopm/blob/dev/service/src/geopm/PluginFactory.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       void PluginFactory<T>::register_plugin(const string &plugin_name,
                                              function<unique_ptr<T>()> make_plugin,
                                              const map<string, string> &dictionary);

       unique_ptr<T> PluginFactory<T>::make_plugin(const string &plugin_name) const;

       vector<string> PluginFactory<T>::plugin_names(void) const;

       const map<string, string>& PluginFactory<T>::dictionary(const string &plugin_name) const;

Description
-----------

GEOPM can be extended though ``Agent``\ , ``IOGroup``\ , and ``Comm`` plugins.
This man page describes the steps for adding a plugin.  Refer to
:doc:`geopm::Agent(3) <geopm::Agent.3>`\ , and :doc:`geopm::IOGroup(3) <geopm::IOGroup.3>` for
more details about these interfaces.  Any **C++** class that derives from
one of these plugin base classes and is compiled into a shared object
file can be loaded at application launch time through the GEOPM plugin
interface.  This allows users and system administrators to extend the
features and behavior of the monitor and control process that GEOPM
executes without recompiling the GEOPM runtime.

There are several steps to implementing and integrating a plugin into
GEOPM.  First, the base class interface must be completely implemented
by the new class.  In addition to the ``virtual`` methods required by the
interface, there are several recommended ``static`` class methods,
described below, that should also be implemented by the derived class
to aid in registration with the plugin factory.  Finally, the plugin
must be registered with the appropriate factory at plugin load time.
This process is discussed in more detail in the following sections.
The factory singletons for ``Agent``\ , ``IOGroup``\ , and ``Comm`` plugins are
available through their respective single accessor functions:
``geopm::agent_factory()``\ , ``geopm::iogroup_factory()``\ , and
``geopm::comm_factory()``.

A good starting point for implementing a new ``Agent`` or ``IOGroup``
plugin is to modify the examples found in the `Agent tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/agent>`_ and
the `IOGroup tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/iogroup>`_.
This code is located in the GEOPM source under ``tutorial/agent`` and
``tutorial/iogroup`` respectively.

Terms
-----

Below are some definitions of terms that are used to describe
different parts of the GEOPM runtime.  Understanding these terms will
help to interpret the documentation about how to extend the GEOPM
runtime.  These are arranged from from highest levels of abstraction
down to the lowest levels of abstraction.


**launcher**\ :
  Wrapper of system application launch (e.g. ``srun`` or ``aprun``) that
  executes the GEOPM runtime with the application.

**report**\ :
  Text file containing summary of aggregated stats collected during
  application run.

**trace**\ :
  Time series of signals collected over application run in a pipe
  separated ASCII table.

**agent**\ :
  Implementation used by GEOPM to interpret sampled data and
  enforce the appropriate controls.

**policy**\ :
  Array of floating-point settings for ``Agent``\ -specific control
  parameters in SI units.

**sample**\ :
  Array of floating-point values providing ``Agent``\ -specific runtime
  statistics in SI units.

**endpoint**\ :
  Interface between resource manager and GEOPM runtime.  This feature
  is still under development and is only available when GEOPM is compiled
  with the ``--enable-beta`` flag.

**profile**\ :
  Interface for annotating compute application; provides ``PlatformIO``
  region signals.

**controller**\ :
  Thread on each compute node that loads plugins and runs GEOPM
  algorithm.

**level**\ :
  Attribute of an ``Agent`` describing the number of edges between it
  and the nearest leaf ``Agent`` in the communication tree (a leaf
  ``Agent`` is *level* zero).

**signal**\ :
  Named parameter in SI units that can be measured using ``PlatformIO``.

**control**\ :
  Named parameter in SI units that can be set using ``PlatformIO``.

Factory Class Methods
---------------------


``register_plugin()``
  Add a plugin to this factory.  The *plugin_name* parameter will be used to request plugins of the
  registered type.  The *make_plugin* parameter is a function that
  returns a new object of the registered type.  The *dictionary* is
  an optional string-to-string dictionary containing static
  information about the registered type.

``make_plugin()``
  Creates an object of the registered type.  If the type was not previously registered, an exception will be thrown.  The
  *plugin_name* parameter will be used to look up the constructor function
  used to create the object. Returns a ``unique_ptr`` to the created object.
  The caller owns the created object.

``plugin_names()``
  Returns a list of all valid plugin names that have been registered with this factory.

``dictionary()``
  Returns an optional dictionary of static metadata about a registered type.  If the type was not registered, an exception is thrown.
  The *plugin_name* parameter is used to look up the desired dictionary.

Building A Plugin Shared Object
-------------------------------

A GEOPM plugin is a shared object file that is loaded at runtime by
the *controller* through the `dlopen(3) <https://man7.org/linux/man-pages/man3/dlopen.3.html>`_ interface.  Each file
provides a new implementation for one of the three extensible classes:
``IOGroup``\ , ``Agent``\ , and ``Comm``.  Each implementation is identified by
a unique name string referred to as the *plugin_name*.  An exception
will be thrown if more than one plugin of the same name and same type
are loaded.  The *plugin_name* is standardized to be all lower case
letters.  The shared object file names must conform to the pattern:

.. code-block::

   libgeopm<CLASS>_<NAME>.so.2.0.0


Here ``<NAME>`` is the *plugin_name* and ``<CLASS>`` is one of the three
strings identifying the plugin type: ``"iogroup"``, ``"agent"``, or ``"comm"``.
The current GEOPM ABI version is ``1.0.0``, and the file name must end
with this string.  Plugins must be marked to have exactly the same ABI
version as the GEOPM library they are intended to be loaded by.  Do
not link the plugin shared object against any of the GEOPM libraries;
this will cause a circular link dependency.  Compile the shared object
with flags appropriate for a dynamically loaded library, e.g. for
``g++`` and ``icpx`` you must provide the ``-fPIC`` and ``-shared`` options.

Plugin Search Path And Load Order
---------------------------------

The ``GEOPM_PLUGIN_PATH`` is a colon-separated list of directories
that contain plugin shared object files to be loaded by the GEOPM
runtime.  Any symlink that resides in a directory specified by this
variable will be ignored.  See :doc:`geopm(7) <geopm.7>` for details
about ``GEOPM_PLUGIN_PATH``.  Note that an Exception will be thrown by
the ``register_plugin()`` method if an attempt is made to register a
plugin with the same name as a previously registered plugin.

In the case of ``IOGroup`` plugins, the most recently loaded plugin to
register a signal or control name provides the implementation at
runtime, even if an earlier ``IOGroup`` plugin had provided a signal or
control with the same name.  The plugins in the ``GEOPM_PLUGIN_PATH``
are loaded in reverse (right to left) order so that plugins earlier in
the search path from left to right are preferred when looking up
signal and control implementations.  The default search path
(\ ``<PREFIX>/lib/geopm``\ ) will have the lowest priority.

For example, if ``GEOPM_PLUGIN_PATH`` is set using the exports below,
the plugins in ``$HOME/plugin/iogroup`` will be used with the highest
priority to provide signal and control names, followed by the plugins
in ``$GEOPM_HOME/tutorial/iogroup``.  Plugins in the default path will
only be used if no higher priority implementation is found.  A more
detailed example of plugin load order can be found in
``tutorial/plugin_load``.

.. code-block:: bash

       export GEOPM_PLUGIN_PATH=$GEOPM_HOME/tutorial/iogroup
       export GEOPM_PLUGIN_PATH=$HOME/plugin/iogroup:$GEOPM_PLUGIN_PATH

.. warning::

    It is imperative that any dynamically loaded library that is a dependency
    of a plugin is available in the environment of the systemd initiated geopmd
    service.  If dependent libraries are not available on the root user's
    LD_LIBRARY_PATH, the plugin will fail to load.
    For this reason, it is recommended that any plugins are compiled with
    system installed gcc/g++ compilers.

Plugin Load Constructor Function
--------------------------------

The shared object file must provide a function that is decorated with
the ``constructor`` compiler directive.  The ``__attribute__((constructor))``
enables the registration of plugins when the shared object is loaded
by a call to `dlopen(3) <https://man7.org/linux/man-pages/man3/dlopen.3.html>`_.
Please see the ``gcc`` documentation for the
`constructor attribute <https://gcc.gnu.org/onlinedocs/gcc-4.3.0/gcc/Function-Attributes.html>`_.

Plugin Class Static Methods
---------------------------

It is recommended that each class deriving from one of the GEOPM
plugin classes implement two static helper methods called
``plugin_name()`` and ``make_plugin()``.  These functions can be used to
provide the inputs to ``PluginFactory::register_plugin()``.  Note that
the first argument to ``register_plugin()`` is a ``string``, i.e. the result
of calling ``plugin_name()``, whereas the ``make_plugin()`` function itself is
passed as the second argument.  The ``make_plugin()`` function should take
no arguments and returns a ``unique_ptr`` to an object of the derived
class.  The ``plugin_name()`` function should take no arguments and return a
``string`` specifying the name of the plugin.  The process for registering
``IOGroup`` and ``Comm`` plugins is identical other than the factory singleton
name and is shown in the example below.  In the case of ``Agent`` plugins,
additional metadata is passed in the form of a dictionary as the third
argument to ``register_plugin()``.  This dictionary is used by ``Agent``
class helper methods to look up information about the sample and
policy names required by the ``Agent``.

EXAMPLE: REGISTER IOGROUP PLUGIN
--------------------------------

Please see the `IOGroup tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/iogroup>`_ for more
information.  This code is located in the GEOPM source under ``tutorial/iogroup``.

.. code-block:: c++

       // This example shows how to register an IOGroup plugin
       #include <geopm/IOGroup.hpp> // geopm::IOGroup,
                                    // geopm::iogroup_factory
       #include <geopm/Helper.hpp>  // geopm::make_unique

       // Header providing class ExampleIOGroup interface
       #include "ExampleIOGroup.hpp"

       // Called during dlopen() to register plugin
       static void __attribute__((constructor))
       register_plugin_example_iogroup(void)
       {
           geopm::PluginFactory<geopm::IOGroup> &iof =
               geopm::iogroup_factory();
           iof.register_plugin(ExampleIOGroup::plugin_name(),
                               ExampleIOGroup::make_plugin);
       }

       // Static method used by the factory to create objects
       std::unique_ptr<IOGroup> ExampleIOGroup::make_plugin(void)
       {
           return geopm::make_unique<ExampleIOGroup>();
       }

       // Static method providing unique plugin name
       std::string ExampleIOGroup::plugin_name(void)
       {
           return "example";
       }

EXAMPLE: REGISTER AGENT PLUGIN
------------------------------

Please see the `Agent tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/agent>`_ for more
information.  This code is located in the GEOPM source under ``tutorial/agent``.

.. code-block:: c++

       // This example shows how to register an Agent plugin
       #include <geopm/Agent.hpp>  // geopm::Agent,
                                   // geopm::agent_factory
       #include <geopm/Helper.hpp> // geopm::make_unique

       // Header providing class ExampleAgent interface
       #include "ExampleAgent.hpp"

       // Called during dlopen() to register plugin
       static void __attribute__((constructor))
       register_plugin_example_agent(void)
       {
           geopm::PluginFactory<geopm::Agent> &af =
               geopm::agent_factory();
           af.register_plugin(ExampleAgent::plugin_name(),
                              ExampleAgent::make_plugin,
                              geopm::Agent::make_dictionary(
                                  ExampleAgent::policy_names(),
                                  ExampleAgent::sample_names()));
       }

       // Static method used by the factory to create objects
       std::unique_ptr<geopm::Agent> ExampleAgent::make_plugin(void)
       {
           return geopm::make_unique<ExampleAgent>();
       }

       // Static method providing unique plugin name
       std::string ExampleAgent::plugin_name(void)
       {
           return "example";
       }

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Agent(3) <geopm::Agent.3>`\ ,
:doc:`geopm::IOGroup(3) <geopm::IOGroup.3>`\ ,
`dlopen(3) <https://man7.org/linux/man-pages/man3/dlopen.3.html>`_
