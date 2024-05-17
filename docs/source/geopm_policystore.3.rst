
geopm_policystore(3) -- GEOPM resource policy store interface
===============================================================


Synopsis
--------

#include `<geopm_policystore.h> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/geopm_policystore.h>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c

       int geopm_policystore_connect(const char *data_path);

       int geopm_policystore_disconnect();

       int geopm_policystore_get_best(const char *agent_name,
                                      const char *profile_name,
                                      size_t max_policy_vals,
                                      double *policy_vals);

       int geopm_policystore_set_best(const char *agent_name,
                                      const char *profile_name,
                                      size_t num_policy_vals,
                                      const double *policy_vals);

       int geopm_policystore_set_default(const char *agent_name,
                                         size_t num_policy_vals,
                                         const double *policy_vals);

Description
-----------

These interfaces expose records of best known policies for profiles used with agents.
The records include the best reported policies, as well as the default policies
to apply when a best run has not yet been recorded.  Policies are shared as
arrays of doubles.  See ``geopm_agent_policy_json_partial()`` in :doc:`geopm_agent(3) <geopm_agent.3>`
for information about interpreting them as json strings.


``geopm_policystore_connect()``
  Connects to a data store at the path specified in *data_path*, creating a
  new one if necessary.  Returns zero on success, or an error code on failure.

``geopm_policystore_disconnect()``
  Disconnects the data store if one is connected.

``geopm_policystore_get_best()``
  Gets the best known policy for a given *profile_name* and *agent_name* in
  the policy store.  Gets the agent's default policy if no best policy has
  been reported.  Returns ``GEOPM_ERROR_INVALID`` if not connected to a store, or
  if *policy_vals* would be truncated.  Returns ``GEOPM_ERROR_DATA_STORE`` if no
  applicable policy exists in the data store, or if any data store errors
  occur.  Returns zero on success, and up to *max_policy_vals* values of the
  found policy are written to *policy_vals*.

``geopm_policystore_set_best()``
  Sets the best known policy for a given *profile_name* and *agent_name* to
  *policy_vals* in the store.  *num_policy_vals* specifies how many values to
  store.  Returns ``GEOPM_ERROR_INVALID`` if not connected to a store. Returns
  ``GEOPM_ERROR_DATA_STORE`` if any data store errors occur. Returns zero on
  success.

``geopm_policystore_set_default()``
  Sets the default policy for a given *agent_name* to *policy_vals* in
  the store.  *num_policy_vals* specifies how many values to store. Returns
  ``GEOPM_ERROR_INVALID`` if not connected to a store.  Returns
  ``GEOPM_ERROR_DATA_STORE`` if any data store errors occur.  Returns zero on
  success.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_agent(3) <geopm_agent.3>`\ ,
:doc:`geopm_error(3) <geopm_error.3>`
