#!/usr/bin/env python3
import cffi

ffibuilder = cffi.FFI()
ffibuilder.set_source('_libgeopmd_py_cffi',
                      r'''
                      #include <geopm_pio.h>
                      #include <geopm_error.h>
                      #include <geopm_shmem.h>
                      #include <geopm_topo.h>
                      #include <geopm_hash.h>
                      #include <geopm_stats_collector.h>
                      #include <geopm_access.h>
                      ''',
                      libraries=['geopmd'])

# geopm_pio.h
ffibuilder.cdef("""
struct geopm_request_s {
    int domain_type;
    int domain_idx;
    char name[255];
};

int geopm_pio_num_signal_name(void);

int geopm_pio_signal_name(int name_idx,
                          size_t result_max,
                          char *result);

int geopm_pio_num_control_name(void);

int geopm_pio_control_name(int name_index,
                           size_t result_max,
                           char *result);

int geopm_pio_signal_domain_type(const char *signal_name);

int geopm_pio_control_domain_type(const char *control_name);

int geopm_pio_read_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx,
                          double *result);

int geopm_pio_write_control(const char *control_name,
                            int domain_type,
                            int domain_idx,
                            double setting);

int geopm_pio_push_signal(const char *signal_name,
                          int domain_type,
                          int domain_idx);

int geopm_pio_push_control(const char *control_name,
                           int domain_type,
                           int domain_idx);

int geopm_pio_sample(int signal_idx,
                     double *result);

int geopm_pio_adjust(int control_idx,
                     double setting);

int geopm_pio_read_batch(void);

int geopm_pio_write_batch(void);

int geopm_pio_save_control(void);

int geopm_pio_save_control_dir(const char *save_dir);

int geopm_pio_restore_control(void);

int geopm_pio_restore_control_dir(const char *save_dir);

int geopm_pio_signal_description(const char *signal_name,
                                 size_t description_max,
                                 char *description);

int geopm_pio_control_description(const char *control_name,
                                  size_t description_max,
                                  char *description);

int geopm_pio_signal_info(const char *signal_name,
                          int *aggregation_type,
                          int *format_type,
                          int *behavior_type);

int geopm_pio_start_batch_server(int client_pid,
                                 int num_signal,
                                 const struct geopm_request_s *signal_config,
                                 int num_control,
                                 const struct geopm_request_s *control_config,
                                 int *server_pid,
                                 int key_size,
                                 char *server_key);

int geopm_pio_stop_batch_server(int server_pid);

int geopm_pio_format_signal(double signal,
                            int format_type,
                            size_t result_max,
                            char *result);

void geopm_pio_reset(void);
""")

# geopm_error.h
ffibuilder.cdef("""
enum geopm_error_e {
    GEOPM_ERROR_RUNTIME = -1,
    GEOPM_ERROR_LOGIC = -2,
    GEOPM_ERROR_INVALID = -3,
    GEOPM_ERROR_FILE_PARSE = -4,
    GEOPM_ERROR_LEVEL_RANGE = -5,
    GEOPM_ERROR_NOT_IMPLEMENTED = -6,
    GEOPM_ERROR_PLATFORM_UNSUPPORTED = -7,
    GEOPM_ERROR_MSR_OPEN = -8,
    GEOPM_ERROR_MSR_READ = -9,
    GEOPM_ERROR_MSR_WRITE = -10,
    GEOPM_ERROR_AGENT_UNSUPPORTED = -11,
    GEOPM_ERROR_AFFINITY = -12,
    GEOPM_ERROR_NO_AGENT = -13,
};

void geopm_error_message(int err, char *msg, size_t size);
""")

# geopm_shmem.h
ffibuilder.cdef("""
    int geopm_shmem_create_prof(const char *shm_key, size_t size, int pid, int uid, int gid);
    int geopm_shmem_path_prof(const char *shm_key, int pid, int uid, size_t shm_path_max, char *shm_path);
""")

# geopm_topo.h
ffibuilder.cdef("""
enum geopm_domain_e {
    GEOPM_DOMAIN_INVALID = -1,
    GEOPM_DOMAIN_BOARD = 0,
    GEOPM_DOMAIN_PACKAGE = 1,
    GEOPM_DOMAIN_CORE = 2,
    GEOPM_DOMAIN_CPU = 3,
    GEOPM_DOMAIN_MEMORY = 4,
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_MEMORY = 5,
    GEOPM_DOMAIN_NIC = 6,
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_NIC = 7,
    GEOPM_DOMAIN_GPU = 8,
    GEOPM_DOMAIN_PACKAGE_INTEGRATED_GPU = 9,
    GEOPM_DOMAIN_GPU_CHIP = 10,
    GEOPM_NUM_DOMAIN = 11,
};

int geopm_topo_num_domain(int domain_type);

int geopm_topo_domain_idx(int domain_type,
                          int cpu_idx);

int geopm_topo_num_domain_nested(int inner_domain,
                                 int outer_domain);

int geopm_topo_domain_nested(int inner_domain,
                             int outer_domain,
                             int outer_idx,
                             size_t num_domain_nested,
                             int *domain_nested);

int geopm_topo_domain_name(int domain_type,
                           size_t domain_name_max,
                           char *domain_name);

int geopm_topo_domain_type(const char *domain_name);

int geopm_topo_create_cache(void);
""")

# geopm_hash.h
ffibuilder.cdef("""
uint64_t geopm_crc32_str(const char *key);
""")

# geopm_stats_collector.h
ffibuilder.cdef("""
struct geopm_request_s;
struct geopm_stats_collector_s;

enum geopm_sample_stats_e {
    GEOPM_SAMPLE_TIME_TOTAL,
    GEOPM_SAMPLE_COUNT,
    GEOPM_SAMPLE_PERIOD_MEAN,
    GEOPM_SAMPLE_PERIOD_STD,
    GEOPM_NUM_SAMPLE_STATS
};

enum geopm_metric_stats_e {
    GEOPM_METRIC_COUNT,
    GEOPM_METRIC_FIRST,
    GEOPM_METRIC_LAST,
    GEOPM_METRIC_MIN,
    GEOPM_METRIC_MAX,
    GEOPM_METRIC_MEAN,
    GEOPM_METRIC_STD,
    GEOPM_NUM_METRIC_STATS,
};

struct geopm_metric_stats_s {
    char name[255];
    double stats[GEOPM_NUM_METRIC_STATS];
};

struct geopm_report_s {
    char host[255];
    char sample_time_first[255];
    double sample_stats[GEOPM_NUM_SAMPLE_STATS];
    size_t num_metric;
    struct geopm_metric_stats_s *metric_stats;
};

int geopm_stats_collector_create(size_t num_requests, const struct geopm_request_s *requests,
                                 struct geopm_stats_collector_s **collector);

int geopm_stats_collector_update(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_update_count(const struct geopm_stats_collector_s *collector,
                                       size_t *update_count);

int geopm_stats_collector_report_yaml(const struct geopm_stats_collector_s *collector,
                                      size_t *max_report_size, char *report_yaml);

int geopm_stats_collector_report(const struct geopm_stats_collector_s *collector,
                                 size_t num_requests, struct geopm_report_s *report);

int geopm_stats_collector_reset(struct geopm_stats_collector_s *collector);

int geopm_stats_collector_free(struct geopm_stats_collector_s *collector);

""")

# geopm_access.h
ffibuilder.cdef("""
int geopm_msr_allowlist(size_t result_max, char *result);
""")

if __name__ == "__main__":
    ffibuilder.compile(verbose=True)
