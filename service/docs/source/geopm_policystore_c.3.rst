.. role:: raw-html-m2r(raw)
   :format: html


geopm_policystore_c(3) -- geopm resource policy store interface
===============================================================






SYNOPSIS
--------

#include `<geopm_policystore.h> <https://github.com/geopm/geopm/blob/dev/src/geopm_policystore.h>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``int geopm_policystore_connect(``\ :
  `const char *`_data\ *path*\ ``);``

* 
  ``int geopm_policystore_disconnect();``

* 
  ``int geopm_policystore_get_best(``\ :
  `const char *`_agent\ *name*\ ``,``\ :raw-html-m2r:`<br>`
  `const char *`_profile\ *name*\ ``,``\ :raw-html-m2r:`<br>`
  ``size_t`` _max_policy\ *vals*\ ``,``\ :raw-html-m2r:`<br>`
  `double *`_policy\ *vals*\ ``);``

* 
  ``int geopm_policystore_set_best(``\ :
  `const char *`_agent\ *name*\ ``,``\ :raw-html-m2r:`<br>`
  `const char *`_profile\ *name*\ ``,``\ :raw-html-m2r:`<br>`
  `size_t `_num_policy\ *vals*\ ``,``\ :raw-html-m2r:`<br>`
  `const double *`_policy\ *vals*\ ``);``

* 
  ``int geopm_policystore_set_default(``\ :
  `const char *`_agent\ *name*\ ``,``\ :raw-html-m2r:`<br>`
  `size_t `_num_policy\ *vals*\ ``,``\ :raw-html-m2r:`<br>`
  `const double *`_policy\ *vals*\ ``);``

DESCRIPTION
-----------

These interfaces expose records of best known policies for profiles used with agents.
The records include the best reported policies, as well as the default policies
to apply when a best run has not yet been recorded.  Policies are shared as
arrays of doubles.  See ``geopm_agent_policy_json_partial()`` in **geopm_agent(3)**
for information about interpreting them as json strings.


* 
  ``geopm_policystore_connect``\ ():
  Connects to a data store at the path specified in _data\ *path*\ , creating a
  new one if necessary.  Returns zero on success, or an error code on failure.

* 
  ``geopm_policystore_disconnect``\ ():
  Disconnects the data store if one is connected.

* 
  ``geopm_policystore_get_best``\ ():
  Gets the best known policy for a given _profile\ *name* and _agent\ *name* in
  the policy store.  Gets the agent's default policy if no best policy has
  been reported.  Returns GEOPM_ERROR_INVALID if not connected to a store, or
  if _policy\ *vals* would be truncated.  Returns GEOPM_ERROR_DATA_STORE if no
  applicable policy exists in the data store, or if any data store errors
  occur.  Returns zero on success, and up to _max_policy\ *vals* values of the
  found policy are written to _policy\ *vals*.

* 
  ``geopm_policystore_set_best``\ ():
  Sets the best known policy for a given _profile\ *name* and _agent\ *name* to
  _policy\ *vals* in the store.  _num_policy\ *vals* specifies how many values to
  store.  Returns GEOPM_ERROR_INVALID if not connected to a store. Returns
  GEOPM_ERROR_DATA_STORE if any data store errors occur. Returns zero on
  success.

* 
  ``geopm_policystore_set_default``\ ():
  Sets the default policy for a given _agent\ *name* to _policy\ *vals* in
  the store.  _num_policy_vals specifies how many values to store. Returns
  GEOPM_ERROR_INVALID if not connected to a store.  Returns
  GEOPM_ERROR_DATA_STORE if any data store errors occur.  Returns zero on
  success.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_\ ,
**geopm_agent(3)**\ ,
`geopm_error(3) <geopm_error.3.html>`_
