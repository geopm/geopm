.. role:: raw-html-m2r(raw)
   :format: html


geopm::MSRIOGroup -- IOGroup providing MSR-based signals and controls
=====================================================================






SYNOPSIS
--------

#include `<geopm/MSRIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/src/MSRIOGroup.hpp>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``virtual set<string> signal_names(``\ :
  ``void) const = 0``\ ;

* 
  ``virtual set<string> control_names(``\ :
  ``void) const = 0``\ ;

* 
  ``virtual bool is_valid_signal(``\ :
  `const string &`_signal\ *name*\ ``) const = 0;``

* 
  ``virtual bool is_valid_control(``\ :
  `const string &`_control\ *name*\ ``) const = 0;``

* 
  ``virtual int signal_domain_type(``\ :
  `const string &`_signal\ *name*\ ``) const = 0;``

* 
  ``virtual int control_domain_type(``\ :
  `const string &`_control\ *name*\ ``) const = 0;``

* 
  ``virtual int push_signal(``\ :
  `const string &`_signal\ *name*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``) = 0``\ ;

* 
  ``virtual int push_control(``\ :
  `const string &`_control\ *name*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``) = 0``\ ;

* 
  ``virtual void read_batch(``\ :
  ``void) = 0``\ ;

* 
  ``virtual void write_batch(``\ :
  ``void) = 0``\ ;

* 
  ``virtual double sample(``\ :
  ``int`` _sample\ *idx*\ ``) = 0;``

* 
  ``virtual void adjust(``\ :
  ``int`` _control\ *idx*\ ``,`` :raw-html-m2r:`<br>`
  ``double`` *setting*\ ``) = 0;``

* 
  ``virtual double read_signal(``\ :
  `const string &`_signal\ *name*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``) = 0;``

* 
  ``virtual void write_control(``\ :
  `const string &`_control\ *name*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *type*\ ``,`` :raw-html-m2r:`<br>`
  ``int`` _domain\ *idx*\ ``,`` :raw-html-m2r:`<br>`
  ``double`` *setting*\ ``) = 0;``

* 
  ``virtual void save_control(``\ :
  ``void) = 0``\ ;

* 
  ``virtual void restore_control(``\ :
  ``void) = 0``\ ;

* 
  ``virtual function<double(const vector<double> &)> agg_function(``\ :
  `const string &`_signal\ *name*\ ``) const = 0;``

* 
  ``virtual string signal_description(``\ :
  `const string &`_signal\ *name*\ ``) const = 0;``

* 
  ``virtual string control_description(``\ :
  `const string &`_control\ *name*\ ``) const = 0;``

* 
  ``std::string msr_allowlist(``\ :
  ``void) const;``

* 
  ``std::string msr_allowlist(``\ :
  `int `_cpuid_\ ``) const;``

* 
  ``int cpuid(``\ :
  ``void) const;``

* 
  ``void register_msr_signal(``\ :
  `const std::string &`_signal\ *name*\ ``);``

* 
  ``void register_msr_control(``\ :
  `const std::string &`_control\ *name*\ ``);``

* 
  ``static std::string plugin_name(``\ :
  ``void);``

* 
  ``static std::unique_ptr<IOGroup> make_plugin(``\ :
  ``void);``

DESCRIPTION
-----------

The MSRIOGroup implements the `geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_ interface to
provide hardware signals and controls using MSRs on Intel platforms.
It relies on **geopm::MSR(3)** and `geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3.html>`_.

CLASS METHODS
-------------


* 
  ``signal_names``\ ():
  Returns the list of signal names provided by this IOGroup.  This
  includes aliases for common hardware-based signals such as
  CPU_FREQUENCY_STATUS, as well as the supported MSRs for the current platform.

* 
  ``control_names``\ ():
  Returns the list of control names provided by this IOGroup.  This
  includes aliases for common hardware-based controls such as
  CPU_FREQUENCY_CONTROL, as well as the supported MSRs for the current platform.

* 
  ``is_valid_signal``\ ():
  Returns whether the given _signal\ *name* is supported by the
  MSRIOGroup for the current platform.  Note that different
  platforms may have different supported MSRs.

* 
  ``is_valid_control``\ ():
  Returns whether the given _control\ *name* is supported by the
  MSRIOGroup for the current platform.  Note that different
  platforms may have different supported MSRs.

* 
  ``signal_domain_type``\ ():
  Returns the domain type for the the signal specified by
  _signal\ *name*.  The domain for a signal may be different on
  different platforms.

* 
  ``control_domain_type``\ ():
  Returns the domain type for the the control specified by
  _control\ *name*.  The domain for a control may be different on
  different platforms.

* 
  ``push_signal``\ ():
  Adds the signal specified by _signal\ *name* for _domain\ *type* at
  index _domain\ *idx* to the list of signals to be read during
  read_batch().  If the domain of a signal spans multiple Linux
  logical CPUs, only one CPU from that domain will be read, since
  all CPUs from the same domain and index will return the same
  value.

* 
  ``push_control``\ ():
  Adds the control specified by _control\ *name* for _domain\ *type* at
  index _domain\ *idx* to the list of controls to be written during
  write_batch().  If the domain of a control spans multiple Linux
  logical CPUs, values written to that control will be written to
  all CPUs in the domain.

* 
  ``read_batch``\ ():
  Sets up `geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3.html>`_ for batch reading if needed, then reads
  all pushed signals through the MSRIO::read_batch() method.

* 
  ``write_batch``\ ():
  Writes all adjusted values through the `geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3.html>`_
  write_batch() method.

* 
  ``sample``\ ():
  Returns the value of the signal specified by a _signal\ *idx*
  returned from push_signal().  The value will have been updated by
  the most recent call to read_batch().

* 
  ``adjust``\ ():
  Sets the control specified by a _control\ *idx* returned from
  push_control() to the value *setting*.  The value will be written
  to the underlying MSR by the next call to write_batch().

* 
  ``read_signal``\ ():
   Immediately read and decode the underlying MSR supporting
  _signal\ *name* for _domain\ *type* at index _domain\ *idx* and return
  the result in SI units.

* 
  ``write_control``\ ():
  Immediately encode the SI unit value *setting* and write the
  correct bits of the MSR supporting _control\ *name* for
  _domain\ *type* at _domain\ *idx*.

* 
  ``save_control``\ ():
  Attempts to read and save the current values of all control MSRs
  for the platform.  If any control is not able to be read, it will
  be skipped.

* 
  ``restore_control``\ ():
  Using the values saved by save_control(), attempts to write back
  the original values of all control MSRs.  Any control that is not
  able to be written will be skipped.

* 
  ``agg_function``\ ():
  Returns the function that should be used to aggregate
  _signal\ *name*.  If one was not previously specified by this class,
  the default function is select_first from `geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_.

* 
  ``signal_description``\ ():
  Returns a string description for _signal\ *name*\ , if defined.
  Further descriptions of MSR signals can be found in The Intel
  Software Developer's Manual at
  https://software.intel.com/en-us/articles/intel-sdm

* 
  ``control_description``\ ():
  Returns a string description for _control\ *name*\ , if defined.
  Further descriptions of MSR controls can be found in The Intel
  Software Developer's Manual at
  https://software.intel.com/en-us/articles/intel-sdm.

* 
  ``signal_behavior``\ ():
  Returns one of the IOGroup::signal_behavior_e values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

* 
  ``msr_allowlist``\ ():
  Fill string with the msr-safe allowlist file contents reflecting
  all known MSRs for the current platform, or if *cpuid* is
  provided, for the platform specified by *cpuid*.  Returns a string
  formatted to be written to an msr-safe allowlist file.

* 
  ``cpuid``\ ():
  Get the cpuid of the current platform.

* 
  ``register_msr_signal``\ ():
  Register a single MSR field as a signal. This is called by
  init_msr().  The _signal\ *name* is a compound signal name of the
  form "msr_name:field_name" where msr_name is the name of the MSR
  and the field_name is the name of the signal field held in the
  MSR.

* 
  ``register_msr_control``\ ():
  Register a single MSR field as a control. This is called by
  init_msr().  The _control\ *name* is a compound control name of the
  form "msr_name:field_name" where msr_name is the name of the MSR
  and the field_name is the name of the control field held in the
  MSR.

* 
  ``plugin_name``\ ():
  Returns the name of the plugin to use when this plugin is
  registered with the IOGroup factory; see
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

* 
  ``make_plugin``\ ():
  Returns a pointer to a new MSRIOGroup object; see
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

ENUMERATIONS
------------


* ``enum m_cpuid_e``\ :
  Contains the list of currently-supported cpuid values.  The cpuid
  can be determined by running ``lscpu`` and appending the CPU family
  in hex to the Model in hex.

ENVIRONMENT
-----------

If the ``GEOPM_PLUGIN_PATH`` environment variable is set to a
colon-separated list of paths, the paths will be checked for files
starting with "msr_" and ending in ".json".  The default plugin path
will also be searched.  The MSRIOGroup will attempt to load additional
MSR definitions from any JSON file it finds. The files must follow this
schema:

.. literalinclude:: ../../json_schemas/msrs.schema.json
    :language: json

Refer to the documentation for ``--geopm-plugin-path`` in
`geopmlaunch(1) <geopmlaunch.1.html>`_.



SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_\ ,
**geopm::MSR(3)**\ ,
`geopm::MSRIO(3) <GEOPM_CXX_MAN_MSRIO.3.html>`_\ ,
`geopmlaunch(1) <geopmlaunch.1.html>`_
