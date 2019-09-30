
#ifndef CONFIG_HPP_INCLUDE
#define CONFIG_HPP_INCLUDE

#include <stdlib.h>
#include <stdint.h>
#include <string>
#include <limits.h>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>
#include <functional>

#include "geopm_env.h"
#include "SharedMemoryUser.hpp"
#include "SharedMemoryImp.hpp"
#include "AppConfigData.hpp"


namespace geopm
{
    class PlatformIO;
    class PlatformTopo;

    class ConfigAgent 
    {
        public:
            std::string sample_key_path;
            std::string sample_key;
            std::unique_ptr<geopm::SharedMemory> m_app_ctl_shmem;
            struct app_interface *m_conf;

        public:
            ConfigAgent();
            ~ConfigAgent();
            
            int init_shmem(std::string );
            void init_config();
            bool is_config_explored();
            void set_new_powercap(double);

    };

    class ConfigApp
    {
        public:
            struct app_interface *m_conf;
            std::string sample_key;
            std::unique_ptr<geopm::SharedMemoryUser> m_app_ctl_shmem;
            int m_shm_rank;
            std::vector<int> m_control_idx;

            struct timeval start_t, end_t;
            std::map<regionmapkey, regionprof> regmap;
            PlatformIO &m_platform_io;
            const PlatformTopo &m_platform_topo;
            double m_start_energy;
            double m_end_energy;
            int m_pkg_pwr_domain_type;
            int m_num_pkg;

        public:
            ConfigApp();
            ~ConfigApp();
            int init_shmem(std::string);
            void init_config();
            bool is_config_explored();
            int get_config_explore_num_threads();
            void set_shm_rank(int);
            void set_app_pid();
            void signal_epoch();
            void dump_configurations();
            void start_profile();
            void stop_profile(uint64_t);
            void cleanup();
            void set_power_cap();
            int efficient_thread_count();
            void construct_pareto_list();
//            void set_config();
    };
}


#endif
