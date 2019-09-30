/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cfloat>
#include <cmath>
#include <algorithm>

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
#include <unistd.h>
#include "Helper.hpp"
#include "Config.hpp"
#include "PlatformIO.hpp"
#include "PlatformTopo.hpp"

namespace geopm
{

/**********************************
 * ConfigAgent specification
 */
    ConfigAgent::ConfigAgent() {
        /* todo: Get the number of processes running on this node */

    }

    int ConfigAgent::init_shmem(std::string local_key) {
        sample_key = local_key + configshmkey;
        // Remove shared memory file if one already exists.
        sample_key_path = "/dev/shm" + sample_key;
        (void)unlink(sample_key_path.c_str());
        m_app_ctl_shmem = make_unique<SharedMemoryImp>(sample_key, sizeof(struct app_interface));
        if(!m_app_ctl_shmem) {
            return -1;
        }
        m_conf = (struct app_interface *)m_app_ctl_shmem->pointer();
        if(!m_conf) {
            return -1;
        }
        return 0;
    }

    void ConfigAgent::init_config() {
        m_conf = (struct app_interface *) m_app_ctl_shmem->pointer();

        /* Declare the PID of the balancer so that the instance of OMPT handler
         * launched with the balancer (and shares its PID) does not report
         * itself in the configuration database
         */

        m_conf->balancer_pid = getpid();
        int thriter, pcapiter, prociter, regioniter;
        for(prociter = 0; prociter < MAX_PROCS_PER_NODE; prociter++) {
            for(thriter = 0; thriter < NUMTHREADS; thriter++) {
                for(pcapiter = 0; pcapiter < NUMPCAPS; pcapiter++) {
                    m_conf->config[prociter].threads[thriter*NUMPCAPS+pcapiter] = (thriter%NUMTHREADS)+1;
                    m_conf->config[prociter].pcap[thriter*NUMPCAPS+pcapiter] = ((pcapiter%NUMPCAPS) + 1)*10 + 40;
                }
            }
            m_conf->epochid[prociter] = 0;
            m_conf->configepochs[prociter] = NUMTHREADS * NUMPCAPS;
            m_conf->pmap[prociter] = -1;
        }
//        std::cout << "Balancer: GetPID: << getpid() << "\tGetPPID: " << getppid()) << std::endl;
    }

    bool ConfigAgent::is_config_explored() {
        static bool is_explored = false;
        if(!is_explored) {
            int procit;
            for(procit = 0; procit < MAX_PROCS_PER_NODE; procit++) {
                if(m_conf->epochid[procit] <= m_conf->configepochs[procit]) {
                    return is_explored;
                }
            }
            is_explored = true;
        }
        return is_explored;
    }

    void ConfigAgent::set_new_powercap(double pcap) { 
        m_conf->powercap = pcap;
    }

/**********************************
 * ConfigApp specification
 */

    ConfigApp::ConfigApp() 
        : m_platform_io(platform_io())
        , m_platform_topo(platform_topo())
        , m_pkg_pwr_domain_type(m_platform_io.control_domain_type("POWER_PACKAGE_LIMIT"))
        , m_num_pkg(m_platform_topo.num_domain(m_pkg_pwr_domain_type))
    {
        /* todo: Get the number of processes running on this node */
    }

    int ConfigApp::init_shmem(std::string local_key) {
        sample_key = local_key + configshmkey;
        m_app_ctl_shmem = make_unique<SharedMemoryUserImp>(sample_key, geopm_env_timeout());
        if(!m_app_ctl_shmem) {
            return -1;
        }
        m_conf = (struct app_interface *)m_app_ctl_shmem->pointer();
        if(!m_conf) {
            return -1;
        }
        return 0;
    }

    void ConfigApp::set_shm_rank(int shm_rank) {
        m_shm_rank = shm_rank;
    }

    void ConfigApp::set_app_pid() {
        m_conf->pmap[m_shm_rank] = getpid();
    }

    void ConfigApp::init_config() {
        m_conf = (struct app_interface *)m_app_ctl_shmem->pointer();
        int iter;
        int pid = getpid();
        /* Get the process ID of the rank this is running in the context of */
        for(iter = 0; iter < MAX_PROCS_PER_NODE; iter++) {
            if(m_conf->pmap[iter] == pid) {
                break;
            }
        }
        m_shm_rank = iter;
//        std::cout << "OMP: Rank: " << m_shm_rank << 
//                "\t GetPID: " << getpid()  << 
//                "\t GetPPID: " << getppid() << 
//                "\t Balancer PID: " << m_conf->balancer_pid << 
//                std::endl;

        /* If ConfigApp object is outside the context of the GEOPM controller, 
         * initialize PlatformIO. Otherwise, do not touch the platform.
         */
        if(getpid() != m_conf->balancer_pid) {
            for(int domain_idx = 0; domain_idx < m_num_pkg; ++domain_idx) {
                int control_idx = m_platform_io.push_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_PACKAGE, domain_idx);
                m_control_idx.push_back(control_idx);
            }
        }
    }

    bool ConfigApp::is_config_explored() {
        if(m_conf->epochid[m_shm_rank] < m_conf->configepochs[m_shm_rank]) {
            return false;
        } else {
            return true;
        }
    }

    int ConfigApp::get_config_explore_num_threads() {
//        std::cout << "Rank: " << m_shm_rank << 
//                " Epoch: " << m_conf->epochid[m_shm_rank] <<
//                " Pcap: " <<  m_conf->config[m_shm_rank].pcap[m_conf->epochid[m_shm_rank]] << 
//                " ::: Threads: " <<  m_conf->config[m_shm_rank].threads[m_conf->epochid[m_shm_rank]] <<
//                std::endl;
                        
        return m_conf->config[m_shm_rank].threads[m_conf->epochid[m_shm_rank]];
    }

    void ConfigApp::signal_epoch() {
        m_conf->epochid[m_shm_rank]++;
    }

    void ConfigApp::cleanup() {
        regmap.erase(regmap.begin(), regmap.end());
    }

    void ConfigApp::set_power_cap() {
        for (auto ctl_idx : m_control_idx) {
            m_platform_io.write_control("POWER_PACKAGE_LIMIT", GEOPM_DOMAIN_PACKAGE, 
                            ctl_idx, m_conf->config[m_shm_rank].pcap[m_conf->epochid[m_shm_rank]]);
        }
    }

    int ConfigApp::efficient_thread_count() {
        for(std::pair<regionmapkey, regionprof> mapit : regmap) 
        {
            if(mapit.second.powerUsage <= m_conf->powercap) { 
                return mapit.first.threads;
            }
        }

        /* We're here which means that no optimal thread count 
         * was found. Use the maximum threads.
         */
        return NUMTHREADS;
    }

    void ConfigApp::start_profile() {
        /* If we've completed exploration of all configurations, generate a 
         * Pareto-frontier of the configurations based on the observed
         * power and execution time samples.
         */
    
        if(m_conf->epochid[m_shm_rank] == m_conf->configepochs[m_shm_rank]) {
            construct_pareto_list();
        }
        else if(m_conf->epochid[m_shm_rank] < m_conf->configepochs[m_shm_rank]) {
            m_start_energy = m_platform_io.read_signal("ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
            gettimeofday(&start_t, NULL);
        } 
    }

    void ConfigApp::stop_profile(uint64_t regionid) {
        if(m_conf->epochid[m_shm_rank] < m_conf->configepochs[m_shm_rank]) { 
            struct regionmapkey rkey; 
            rkey.regionid = regionid;
            rkey.threads = m_conf->config[m_shm_rank].threads[m_conf->epochid[m_shm_rank]];
            rkey.pcap = m_conf->config[m_shm_rank].pcap[m_conf->epochid[m_shm_rank]];
            
            struct regionprof rentry;
            //Stop timer
            gettimeofday(&end_t, NULL);
            double elapsedTime = (end_t.tv_sec - start_t.tv_sec) * 1000.0;      // sec to ms
            elapsedTime += (end_t.tv_usec - start_t.tv_usec) / 1000.0;
            rentry.elapsedTime = elapsedTime;
            m_end_energy = m_platform_io.read_signal("ENERGY_PACKAGE", GEOPM_DOMAIN_PACKAGE, 0);
            rentry.powerUsage = 1000.0f * (m_end_energy - m_start_energy)/rentry.elapsedTime;

            regmap.insert(std::pair<regionmapkey, regionprof>(rkey, rentry));
//            std::cout << "RegionID: " << (unsigned long)rkey.regionid <<
//                    " Thread: " << rkey.threads << 
//                    " Pcap: " << rkey.pcap <<
//                    "::: Time: " << rentry.elapsedTime <<
//                    " Power:%0.2f" << rentry.powerUsage <<
//                    std::endl; 
        }              
    } 

    void ConfigApp::dump_configurations() {
        int iter;
        std::ofstream ofile;
        std::string fcinit = "configinit_" + std::to_string(getpid());
        printf("Writing to %s\n", fcinit.c_str());
        ofile.open(fcinit, std::ios::out);
        for(iter = 0; iter < (NUMTHREADS * NUMPCAPS); iter++) {
            ofile << "Thread: " << m_conf->config[m_shm_rank].threads[iter] << \
                     ", Pcap" << m_conf->config[m_shm_rank].pcap[iter] << \
                    "\n";
        }
        ofile.close();
    } 

    void ConfigApp::construct_pareto_list() {
        typedef std::function<bool(std::pair<regionmapkey, regionprof>, std::pair<regionmapkey, regionprof>)> Comparator;
        Comparator paretoFunc = 
            [](std::pair<regionmapkey, regionprof> p ,std::pair<regionmapkey, regionprof> q)
            {
                if((p.second.elapsedTime - q.second.elapsedTime) < 0) //if exec_a < exec_b
                {
                    return 0;                      // a should be before b
                }
                else if((p.second.elapsedTime - q.second.elapsedTime) == 0) 
                {
                    if((p.second.powerUsage - q.second.powerUsage) < 0) // if performance is equal but power_a < power_b
                    {
                        return 0;                   // a should be before b
                    }
                    else
                    {
                        return 1;                   // otherwise b should be before a
                    }
                }
                else 
                {
                    return 1;                       // b should be before a
                }
            };
        std::set<std::pair<regionmapkey, regionprof>, Comparator> powerSamples(
                    regmap.begin(), regmap.end(), paretoFunc);
        for(std::pair<regionmapkey, regionprof> mapit : regmap)
        {
            std::cout << " Region: " << mapit.first.regionid << 
                         " Threads: " << mapit.first.threads <<
                         " Pcap: " << mapit.first.pcap <<
                         " --- Time: " << mapit.second.elapsedTime <<
                         "  Power: " << mapit.second.powerUsage << "\n";
        }
    }
}
