.. role:: raw-html-m2r(raw)
   :format: html


geopm::Agent(3) -- geopm agent plugin interface
===============================================






NAMESPACES
----------

The ``Agent`` class and the ``agent_factory()`` function are members of
the ``namespace geopm``\ , but the full names, ``geopm::Agent`` and
``geopm::agent_factory()``\ , have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

SYNOPSIS
--------

#include `<geopm/Agent.hpp> <https://github.com/geopm/geopm/blob/dev/src/Agent.hpp>`_\ 

Link with ``-lgeopm``


.. code-block:: c++

       PluginFactory<Agent> &agent_factory(void);

       virtual void Agent::init(int level, const vector<int> &fan_in, bool is_level_root) = 0;

       virtual void Agent::validate_policy(vector<double> &policy) const = 0;

       virtual void Agent::split_policy(const vector<double> &in_policy,
                                        vector<vector<double> >& out_policy) = 0;

       virtual bool Agent::do_send_policy(void) const = 0;

       virtual void Agent::aggregate_sample(const vector<vector<double> > &in_sample,
                                            vector<double> &out_sample) = 0;

       virtual bool Agent::do_send_sample(void) const = 0;

       virtual void Agent::adjust_platform(const vector<double> &in_policy) = 0;

       virtual bool Agent::do_write_batch(void) const = 0;

       virtual void Agent::sample_platform(vector<double> &out_sample) = 0;

       virtual void Agent::wait(void) = 0;

       virtual vector<pair<string, string> > Agent::report_header(void) const = 0;

       virtual vector<pair<string, string> > Agent::report_host(void) const = 0;

       virtual map<uint64_t, vector<pair<string, string> > > Agent::report_region(void) const = 0;

       virtual vector<string> Agent::trace_names(void) const = 0;

       virtual vector<function<string(double)> > Agent::trace_formats(void) const;

       virtual void Agent::trace_values(vector<double> &values) = 0;

       virtual void Agent::enforce_policy(const vector<double> &policy) const;

       static int Agent::num_policy(const map<string, string> &dictionary);

       static int Agent::num_policy(const string &agent_name);

       static int Agent::num_sample(const map<string, string> &dictionary);

       static int Agent::num_sample(const string &agent_name);

       static vector<string> Agent::policy_names(const map<string, string> &dictionary);

       static vector<string> Agent::policy_names(const string &agent_name);

       static vector<string> Agent::sample_names(const map<string, string> &dictionary);

       static vector<string> Agent::sample_names(const string &agent_name);

       static map<string, string> Agent::make_dictionary(const vector<string> &policy_names,
                                                         const vector<string> &sample_names);

       static void Agent::aggregate_sample(const vector<vector<double> > &in_sample,
                                           const vector<function<double(const vector<double>&)> > &agg_func,
                                           vector<double> &out_sample);

DESCRIPTION
-----------

The ``Agent`` class is an abstract pure virtual class that defines the
fundamental procedures executed by the GEOPM runtime.  In general, the
Agent is responsible for making decisions about how and what control the
runtime should exert based on readings of system values.  By default
the `geopm_agent_monitor(7) <geopm_agent_monitor.7.html>`_ is used, but other agents can be
selected with the ``--geopm-agent`` command line option to the
**geopm_launcher(1)** or ``GEOPM_AGENT`` environment variable.  Exactly
one agent type is used during each execution of the GEOPM runtime.

The `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_ man page gives a general overview of
concepts related to extending GEOPM through plugins.  Please refer to
that overview as an introduction before implementing an ``Agent`` plugin.

A single process (or application thread) on each compute node utilized
by an application running with GEOPM creates a ``Controller`` object and
each of these objects creates one or more ``Agent`` objects.  The
``Agent`` objects are related to each other through a balanced tree of
bi-directional communication.  The "leaf" ``Agent`` objects are defined
to have no children and one parent.  The "tree" ``Agent`` objects defined
to have many children and one Agent as the parent.  The "root" Agent
objects are defined to have many children and a static policy as the parent.
Note that in some cases (e.g. a single node job execution) a "tree" Agent
may not be involved at all.  The ``Agent::init()`` method is called by the
``Controller`` prior to all other ``Agent`` methods.  The parameters passed
by the ``Controller`` in this call define the geometry of the ``Agent`` tree
and where the particular ``Agent`` object falls in the tree.  See detailed
description of ``Agent::init()`` below for more information about the tree
structure.

All ``Controller``\ s create an ``Agent`` object to execute *level* 0, or
"leaf" responsibilities.  The leaf responsibilities include monitoring
signals and deriving samples to send to their parent ``Agent``\ s at
*level* 1 in the tree.  Additionally a leaf ``Agent`` must interpret
policies received from their parent agent at *level* 1 and set
controls which reflect the policy.  Some of the ``Controller`` objects
will create ``Agent`` objects to execute non-zero *level*\ , or "tree"
responsibilities.  These non-zero *level* ``Agent`` objects are
responsible for aggregating samples from child agents to send to
parent agents and splitting policy values from parent agents to send
to child agents.  Note that the ``Agent`` running at the root of the
tree uses the same policy/sample interface to interact with the
resource manager.

The ``Agent`` class is designed to read signals and write controls for
the hardware and application using the `geopm::PlatformIO(3) <GEOPM_CXX_MAN_PlatformIO.3.html>`_
interface.  Please see the manual for details about how the
``PlatformIO`` abstraction enables access to hardware capabilities,
extension of ``Agent`` algorithms to new hardware architectures, a
mapping of application behavior to hardware domains, and code reuse of
I/O implementations by different ``Agent`` classes.

FACTORY ACCESSOR
----------------


* ``agent_factory()``:
  This method returns the singleton accessor for the Agent factory.
  Calling this method will create the factory if it does not already exist.
  If this method is creating the factory, loading of the built-in Agents
  will be attempted.  For more information see `geopm::PluginFactory(3) <GEOPM_CXX_MAN_PluginFactory.3.html>`_.

CLASS METHODS
-------------


* ``init()``:
  The ``init()`` method is the first method that will be called and
  provides contextual information for the ``Agent`` about the
  communication tree geometry and where in the tree the ``Agent``
  resides.  The communication tree that connects the ``Agent`` objects
  is a balanced tree.  The *level* is the minimum number of edges
  between ``self`` and any leaf ``Agent``.  Only when *level* is zero
  will the ``sample_platform()`` and ``adjust_platform()`` methods be
  called.  If the *level* is zero then the ``init()`` method should
  push all signals and controls for **PlatformIO(3)**.  The *fan_in**
  is a vector indexed by *level* that describes the number of
  siblings that an ``Agent`` at *level* has which share a parent.  The
  figure below represents a tree with ``fan_in == {4,3,2}`` where each
  ``Agent`` is signified by a digit that is equal to the *level*.  Note
  that this example supports 24 compute nodes.  The *is_level_root*
  parameter is true for one child of each parent and only if this
  parameter is true will the controller call the ``ascend()`` or
  ``descend()`` methods of the object.

.. code-block::

                                       (3)
                            ___________/ \____________
                           /                          \
                      __ (2)                         _(2)__
              _______/  /   \_____             _____/    | \_______
             /         |          \           /          |         \
           (1)        (1)        (1)         (1)        (1)        (1)
          -- --      -- --      -- --       -- --      -- --      -- --
         /  |  \    /  |  \    /  |  \     /  |  \    /  |  \    /  |  \
       (0)(0|0)(0)(0)(0|0)(0)(0)(0|0)(0) (0)(0|0)(0)(0)(0|0)(0)(0)(0|0)(0)


* 
  ``validate_policy()``:
  Called by user of Agent class  to validate incoming policy values and
  replace NaNs with defaults.  If a value of *policy* is not NaN but the
  value is not supported by the Agent the method will throw a ``geopm::Exception``
  with error code ``GEOPM_ERROR_INVALID``.

* 
  ``split_policy()``:
  Split policy for children at next level down the tree.  The
  *in_policy* is an input vector of policy values from the parent.
  The *out_policy* is an output vector of policies to be sent to
  each child.

* 
  ``do_send_policy()``:
  Used to indicate to the Controller whether to send the output
  from ``split_policy()`` down the tree to the Agent's children.
  Returns true if the policy has been updated since the last call.

* 
  ``aggregate_sample()``:
  Aggregate samples from children for the next level up the tree.
  The *in_sample* parameter is a vector of sample vectors, one
  sample vector from each child.  The samples from a given index
  in the input vectors are transformed to a single value at the
  same index in the output.  The *out_sample* is an output vector
  of aggregated sample values to be sent up to the parent.

* 
  ``do_send_sample()``:
  Used to indicate to the Controller whether to send the output from
  ``aggregate_sample()`` up the tree to the Agent's parent.  Returns
  true if any samples have been updated since the last call.

* 
  ``adjust_platform()``:
  Adjust the platform settings based on the policy from above.
  Settings for each control are in the *in_policy*.

* 
  ``do_write_batch()``:
  Used to indicate to the Controller whether to call
  ``PlatformIO::write_batch()`` to enact new control values on the
  platform.  Returns true if any control values have been updated
  since the last call.

* 
  ``sample_platform()``:
  Read signals from the platform and interpret/aggregate these
  signals to create a sample which can be sent up the tree.  The
  *out_sample* parameter is an output vector of agent specific sample
  values to be sent up the tree. Returns true if the sample has been
  updated since the last call.

* 
  ``wait()``:
  Called to wait for the sample period to elapse. This controls the
  cadence of the Controller main loop.

* 
  ``report_header()``:
  Custom fields that will be added to the report header when this
  agent is used.  To be consistent with the rest of the header, the
  keys should be title case, e.g. ``"Agent Header Name"``; they must not
  contain the colon character ``':'``.  Care must be taken not to add
  keys that conflict with default header keys, like names that start
  with ``"Start Time"``, ``"Profile"``, ``"Agent"`` or ``"Policy"``.

* 
  ``report_host()``:
  Custom fields for the Host section of the report.  To be
  consistent with the rest of this section, the keys should have
  the first letter capitalized, e.g. ``"Final freq map"``; they must
  not contain the colon character ``':'``.  Care must be taken not to
  add keys that conflict with default host keys, like names that
  start with ``"Region"``, ``"Epoch Totals"`` or ``"Application Totals"``.

* 
  ``report_region()``:
  Custom fields for each region in the report.  To be consistent
  with the rest of the region report, the string keys that will
  appear at the start of each line should be all lower case with
  words separated by hyphens and followed by the units if
  applicable, e.g ``"package-energy (joules)"``.  The field name must
  not contain the colon character ``':'``.  Care must be taken not to
  add keys that conflict with the default region keys, like names
  that start with ``"runtime"``, ``"sync-runtime"``, ``"package-energy"``,
  ``"dram-energy"``, ``"power"``, ``"frequency"``, ``"network-time"``, or ``"count"``.

* 
  ``trace_names()``:
  Column headers to be added to the trace.  These will be
  automatically converted to lower case.  The header names must
  not contain the pipe character ``'|'`` or whitespace.

* 
  ``trace_formats()``:
  Returns format string for each column added to the trace

* 
  ``trace_values()``:
  Called by Controller to get latest values to be added to the trace.

* 
  ``enforce_policy()``:
  Enforce the policy one time with
  ``PlatformIO::write_control()``.  Called to enforce
  static policies in the absence of a Controller.

* 
  ``num_policy()``:
  Used to look up the number of values in the policy vector sent
  down the tree for a specific type of ``Agent``. This should be
  called with the *dictionary* returned by
  ``agent_factory().dictionary(agent_name)`` for the ``Agent`` of
  interest.
  Also has an overloaded version which takes the *agent_name*.
  Note this is a static helper function.

* 
  ``num_sample()``:
  Used to look up the number of values in the sample vector sent up
  the tree for a specific type of ``Agent``. This should be called
  with the dictionary returned by
  ``agent_factory().dictionary(agent_name)`` for the ``Agent`` of
  interest.
  Also has an overloaded version which takes the *agent_name*.
  Note this is a static helper function.

* 
  ``policy_names()``:
  Used to look up the names of values in the policy vector sent down
  the tree for a specific type of ``Agent``. This should be called
  with the dictionary returned by
  ``agent_factory().dictionary(agent_name)`` for the ``Agent`` of
  interest.
  Also has an overloaded version which takes the *agent_name*.
  Note this is a static helper function.

* 
  ``sample_names()``:
  Used to look up the names of values in the sample vector sent up
  the tree for a specific of ``Agent``. This should be called with the
  dictionary returned by
  ``agent_factory().dictionary(agent_name)`` for the ``Agent`` of
  interest.
  Also has an overloaded version which takes the *agent_name*.
  Note this is a static helper function.

* 
  ``make_dictionary()``:
  Used to create a correctly formatted dictionary for an ``Agent`` at
  the time the ``Agent`` is registered with the factory. Concrete
  ``Agent``` classes may provide ``policy_names()`` and ``sample_names()``
  methods to provide the vectors to be passed to this method.  Note
  this is a static helper function.

* 
  ``aggregate_sample()``:
  Generically aggregate a vector of samples given a vector of
  aggregation functions. This helper method applies a different
  aggregation function to each sample element while aggregating
  across child samples. The *in_sample* parameter is an input vector
  over children of the sample vector received from each child.  The
  *agg_func* is an input vector over agent samples of the
  aggregation function that is applied.  The *out_sample* is an
  output sample vector resulting from the applying the aggregation
  across child samples.  Note this is a static helper function.

ERRORS
------

All functions described on this man page throw `geopm::Exception(3) <GEOPM_CXX_MAN_Exception.3.html>`_
on error.

EXAMPLE
-------

Please see the `Agent tutorial <https://github.com/geopm/geopm/tree/dev/tutorial/agent>`_ for more
information.  This code is located in the GEOPM source under tutorial/agent.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_
