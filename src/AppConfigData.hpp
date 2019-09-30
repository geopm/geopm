#ifndef _APP_CONFIG_DATA_H_
#define _APP_CONFIG_DATA_H_

#define NUMTHREADS 17
#define NUMPCAPS 8
#define MAX_PROCS_PER_NODE 2
#define configshmkey "-config"
#define MAX_REGIONS 20

/* Per-process configuration and monitoring data structure */
struct app_config { 
    int threads[(NUMTHREADS * NUMPCAPS)];
    double pcap[(NUMTHREADS * NUMPCAPS)];
};

struct app_interface {
    struct app_config config[MAX_PROCS_PER_NODE];
    int pmap[MAX_PROCS_PER_NODE];
    unsigned long epochid[MAX_PROCS_PER_NODE];
    unsigned long configepochs[MAX_PROCS_PER_NODE];

    int balancer_pid;
    double powercap;
}; 

struct regionmapkey {
    uint64_t regionid;
    int threads;
    double pcap;
};

struct regionprof {
    double elapsedTime;
    double powerUsage;
};

inline bool operator<(regionmapkey const& left, regionmapkey const& right) {
    return std::tie(left.regionid, left.threads, left.pcap) < 
                std::tie(right.regionid, right.threads, right.pcap);
}

#endif
