#!/usr/bin/env python3
import cffi
import sys

ffibuilder = cffi.FFI()

# Check whether optional/beta headers are installed
try:
    ffibuilder.verify(r"#include <geopm_policystore.h>", optional=True)
except Exception:
    have_policystore = False
    print('Note: geopmpy failed to build a wrapper for geopm_policystore.h, which '
          'is an optional header from libgeopm. The build failure can be ignored '
          'if you do not plan to use geopmpy.policy_store.',
          file=sys.stderr)
else:
    have_policystore = True

try:
    ffibuilder.verify(r"#include <geopm_endpoint.h>", optional=True)
except Exception:
    have_endpoint = False
    print('Note: geopmpy failed to build a wrapper for geopm_endpoint.h, which '
          'is an optional header from libgeopm. The build failure can be ignored '
          'if you do not plan to use geopmpy.endpoint.',
          file=sys.stderr)
else:
    have_endpoint = True

includes = ['geopm_agent.h']
if have_policystore:
    includes.append('geopm_policystore.h')
if have_endpoint:
    includes.append('geopm_endpoint.h')

ffibuilder.set_source('_libgeopm_py_cffi',
                      '\n'.join([f'#include <{header}>' for header in includes]),
                      libraries=['geopm'])

# geopm_agent.h
ffibuilder.cdef("""
int geopm_agent_supported(const char *agent_name);

int geopm_agent_num_policy(const char *agent_name,
                           int *num_policy);

int geopm_agent_policy_name(const char *agent_name,
                            int policy_idx,
                            size_t policy_name_max,
                            char *policy_name);

int geopm_agent_policy_json(const char *agent_name,
                            const double *policy_array,
                            size_t json_string_max,
                            char *json_string);

int geopm_agent_num_sample(const char *agent_name,
                           int *num_sample);

int geopm_agent_sample_name(const char *agent_name,
                            int sample_idx,
                            size_t sample_name_max,
                            char *sample_name);

int geopm_agent_num_avail(int *num_agent);

int geopm_agent_name(int agent_idx,
                     size_t agent_name_max,
                     char *agent_name);

int geopm_agent_enforce_policy(void);
""")

if have_policystore:
    ffibuilder.cdef("""
    int geopm_policystore_connect(const char *data_path);

    int geopm_policystore_disconnect();

    int geopm_policystore_get_best(const char* agent_name, const char* profile_name,
                                   size_t max_policy_vals, double* policy_vals);

    int geopm_policystore_set_best(const char* agent_name, const char* profile_name,
                                   size_t num_policy_vals, const double* policy_vals);

    int geopm_policystore_set_default(const char* agent_name,
                                      size_t num_policy_vals, const double* policy_vals);
    """)

if have_endpoint:
    ffibuilder.cdef("""
    struct geopm_endpoint_c;

    int geopm_endpoint_create(const char *endpoint_name,
                              struct geopm_endpoint_c **endpoint);

    int geopm_endpoint_destroy(struct geopm_endpoint_c *endpoint);

    int geopm_endpoint_open(struct geopm_endpoint_c *endpoint);

    int geopm_endpoint_close(struct geopm_endpoint_c *endpoint);

    int geopm_endpoint_agent(struct geopm_endpoint_c *endpoint,
                             size_t agent_name_max,
                             char *agent_name);

    int geopm_endpoint_wait_for_agent_attach(struct geopm_endpoint_c *endpoint,
                                             double timeout);

    int geopm_endpoint_stop_wait_loop(struct geopm_endpoint_c *endpoint);

    int geopm_endpoint_reset_wait_loop(struct geopm_endpoint_c *endpoint);

    int geopm_endpoint_profile_name(struct geopm_endpoint_c *endpoint,
                                    size_t profile_name_max,
                                    char *profile_name);

    int geopm_endpoint_num_node(struct geopm_endpoint_c *endpoint,
                                int *num_node);

    int geopm_endpoint_node_name(struct geopm_endpoint_c *endpoint,
                                 int node_idx,
                                 size_t node_name_max,
                                 char *node_name);

    int geopm_endpoint_write_policy(struct geopm_endpoint_c *endpoint,
                                    size_t num_policy,
                                    const double *policy_array);

    int geopm_endpoint_read_sample(struct geopm_endpoint_c *endpoint,
                                   size_t num_sample,
                                   double *sample_array,
                                   double *sample_age_sec);
    """)

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
