.. role:: raw-html-m2r(raw)
   :format: html


geopm::GPUActivityAgent(3) -- agent for selecting GPU frequency based on GPU compute activity
=========================================================






NAMESPACES
----------

The ``GPUActivityAgent`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::GPUActivityAgent``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``GPUActivityAgent`` class is derived from `geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3>`_ class.

SYNOPSIS
--------

#include `<geopm/GPUActivityAgent.hpp> <https://github.com/geopm/geopm/blob/dev/src/GPUActivityAgent.hpp>`_\

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       void GPUActivityAgent::init(int level, const vector<int> &fan_in, bool is_level_root) override;

       void GPUActivityAgent::validate_policy(vector<double> &policy) const override;

       void GPUActivityAgent::split_policy(const vector<double> &in_policy,
                                               vector<vector<double> > &out_policy) override;

       bool GPUActivityAgent::do_send_policy(void) const override;

       void GPUActivityAgent::aggregate_sample(const vector<vector<double> > &in_sample,
                                                   vector<double> &out_sample) override;

       bool GPUActivityAgent::do_send_sample(void) const override;

       void GPUActivityAgent::adjust_platform(const vector<double> &in_policy) override;

       bool GPUActivityAgent::do_write_batch(void) const override;

       void GPUActivityAgent::sample_platform(vector<double> &out_sample) override;

       void GPUActivityAgent::wait(void) override;

       vector<pair<string, string> > GPUActivityAgent::report_header(void) const override;

       vector<pair<string, string> > GPUActivityAgent::report_host(void) const override;

       map<uint64_t, vector<pair<string, string> > > GPUActivityAgent::report_region(void) const override;

       vector<string> GPUActivityAgent::trace_names(void) const override;

       vector<function<string(double)> > GPUActivityAgent::trace_formats(void) const override;

       void GPUActivityAgent::trace_values(vector<double> &values) override;

       void GPUActivityAgent::enforce_policy(const vector<double> &policy) const override;

       static string GPUActivityAgent::plugin_name(void);

       static unique_ptr<Agent> GPUActivityAgent::make_plugin(void);

       static vector<string> GPUActivityAgent::policy_names(void);

       static vector<string> GPUActivityAgent::sample_names(void);

DESCRIPTION
-----------

The behavior of this agent is described in more detail in the
`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7.html>`_ man page.

For more details on the implementation, see the doxygen
page at <https://geopm.github.io/dox/classgeopm_1_1_gpu_activity_agent.html>.

CLASS METHODS
-------------

**TODO**

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
`geopm_agent_gpu_activity(7) <geopm_agent_gpu_activity.7.html>`_\ ,
`geopm::Agent(3) <GEOPM_CXX_MAN_Agent.3.html>`_
