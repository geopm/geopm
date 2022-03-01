.. role:: raw-html-m2r(raw)
   :format: html


geopm::TimeIOGroup(3) -- IOGroup providing time signals
=======================================================






SYNOPSIS
--------

#include `<geopm/TimeIOGroup.hpp> <https://github.com/geopm/geopm/blob/dev/src/TimeIOGroup.hpp>`_\ 

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
  ``static std::string plugin_name(``\ :
  ``void);``

* 
  ``static std::unique_ptr<IOGroup> make_plugin(``\ :
  ``void);``

DESCRIPTION
-----------

This IOGroup provides an implementation of the TIME signal for the
time since GEOPM startup.

CLASS METHODS
-------------


* 
  ``signal_names``\ ():
  Returns the time signal name, "TIME::ELAPSED", and its alias, "TIME".

* 
  ``control_names``\ ():
  Does nothing; this IOGroup does not provide any controls.

* 
  ``is_valid_signal``\ ():
  Returns true if the _signal_name is one from the list returned by
  signal_names().

* 
  ``is_valid_control``\ ():
  Returns false; this IOGroup does not provide any controls.

* 
  ``signal_domain_type``\ ():
  If the _signal\ *name* is valid for this IOGroup, returns
  M_DOMAIN_BOARD.

* 
  ``control_domain_type``\ ():
  Returns M_DOMAIN_INVALID; this IOGroup does not provide any controls.

* 
  ``push_signal``\ ():
  Since this IOGroup only provides one signal, returns 0 if the _signal\ *name*
  is valid.  The _domain\ *type* and _domain\ *idx* are ignored.

* 
  ``push_control``\ ():
  Should not be called; this IOGroup does not provide any controls.

* 
  ``read_batch``\ ():
  If a time signal has been pushed, updates the time since the
  TimeIOGroup was created.

* 
  ``write_batch``\ ():
  Does nothing; this IOGroup does not provide any controls.

* 
  ``sample``\ ():
  Returns the value of the signal specified by a _signal\ *idx*
  returned from push_signal().  The value will have been updated by
  the most recent call to read_batch().

* 
  ``adjust``\ ():
  Should not be called; this IOGroup does not provide any controls.

* 
  ``read_signal``\ ():
  If _signal\ *name* is valid, immediately return the time since the
  TimeIOGroup was created.

* 
  ``write_control``\ ():
  Should not be called; this IOGroup does not provide any controls.

* 
  ``save_control``\ ():
  Does nothing; this IOGroup does not provide any controls.

* 
  ``restore_control``\ ():
  Does nothing; this IOGroup does not provide any controls.

* 
  ``agg_function``\ ():
  The TIME signal provided by this IOGroup is aggregated using the
  average() function from `geopm::Agg(3) <GEOPM_CXX_MAN_Agg.3.html>`_.

* 
  ``signal_description``\ ():
  Returns a string description for _signal\ *name*\ , if defined.

* 
  ``control_description``\ ():
  Does nothing; this IOGroup does not provide any controls.

* 
  ``signal_behavior``\ ():
  Returns one of the IOGroup::signal_behavior_e values which
  describes about how a signal will change as a function of time.
  This can be used when generating reports to decide how to
  summarize a signal's value for the entire application run.

* 
  ``plugin_name``\ ():
  Returns the name of the plugin to use when this plugin is
  registered with the IOGroup factory; see
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

* 
  ``make_plugin``\ ():
  Returns a pointer to a new TimeIOGroup object; see
  `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ for more details.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm::IOGroup(3) <GEOPM_CXX_MAN_IOGroup.3.html>`_
