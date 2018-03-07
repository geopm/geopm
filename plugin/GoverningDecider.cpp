/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include <hwloc.h>
#include <cmath>
#include <signal.h>
#include <unistd.h>

//#include "geopm.h"
#include "geopm_message.h"
#include "geopm_plugin.h"
#include "GoverningDecider.hpp"
//#include "geopm_error.h"
//#include "geopm_time.h"
//#include "geopm_signal_handler.h"
#include "Exception.hpp"
//#include "config.h"
#include<inttypes.h>

int geopm_plugin_register(int plugin_type, struct geopm_factory_c *factory, void *dl_ptr)
{
    int err = 0;

    try {
        if (plugin_type == GEOPM_PLUGIN_TYPE_DECIDER) {
            geopm::IDecider *decider = new geopm::GoverningDecider;
            geopm_factory_register(factory, decider, dl_ptr);
        }
    }
    catch(...) {
        err = geopm::exception_handler(std::current_exception());
    }

    return err;
}

namespace geopm
{
    GoverningDecider::GoverningDecider()
        : m_name("power_governing")
        , m_min_num_converged(5)
        , m_last_power_budget(DBL_MIN)
        , m_last_dram_power(DBL_MAX)
        , m_num_sample(5)
    {

    }

    GoverningDecider::GoverningDecider(const GoverningDecider &other)
        : Decider(other)
        , m_name(other.m_name)
        , m_min_num_converged(other.m_min_num_converged)
        , m_last_power_budget(other.m_last_power_budget)
        , m_last_dram_power(other.m_last_dram_power)
        , m_num_sample(other.m_num_sample)
    {

    }

    GoverningDecider::~GoverningDecider()
    {

    }

    IDecider *GoverningDecider::clone(void) const
    {
        return (IDecider*)(new GoverningDecider(*this));
    }

    bool GoverningDecider::decider_supported(const std::string &description)
    {
        return (description == m_name);
    }

    const std::string& GoverningDecider::name(void) const
    {
        return m_name;
    }

    bool GoverningDecider::update_policy(const struct geopm_policy_message_s &policy_msg, IPolicy &curr_policy)
    {
        bool result = false;
        if (policy_msg.power_budget != m_last_power_budget) {
            int num_domain = curr_policy.num_domain();
            double split_budget = policy_msg.power_budget / num_domain;
            std::vector<double> domain_budget(num_domain);
            std::fill(domain_budget.begin(), domain_budget.end(), split_budget);
            std::vector<uint64_t> region_id;
            curr_policy.region_id(region_id);
            for (auto region = region_id.begin(); region != region_id.end(); ++region) {
                curr_policy.update((*region), domain_budget);
                auto it = m_num_converged.lower_bound((*region));
                if (it != m_num_converged.end() && (*it).first == (*region)) {
                    (*it).second = 0;
                }
                else {
                    it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>((*region), 0));
                }
                curr_policy.is_converged((*region), false);
            }
            if (m_last_power_budget == DBL_MIN) {
                curr_policy.mode(policy_msg.mode);
                curr_policy.policy_flags(policy_msg.flags);
            }
            m_last_power_budget = policy_msg.power_budget;
            m_last_dram_power = DBL_MAX;
            result = true;
        }
        return result;
    }

#define HFI_POWER_ACCOUNTING_SUPPORT 1

#ifdef HFI_POWER_ACCOUNTING_SUPPORT
// Post-Silicon estimates for WFR(STL-1 generation)
// KNL-f/SKX-F dual port platform running a typical
// workload
#define HFI_IDLE_POWER 5.5 // power at 0GBps with no workload
#define HFI_MAX_POWER 11.9 // power at 100GBps with typical workload 
#define HFI_MAX_BW_GBPS 100
#define HFI_MIN_BW_GBPS 0  

#define OPATOOLS_OUT_MAX_LEN 10
#define NW_LOG_REC_MAX 100


    static volatile unsigned g_is_popen_complete = 0;
    static struct sigaction g_popen_complete_signal_action;

    static void geopm_popen_complete(int signum)
    {
       if (signum == SIGCHLD) {
            g_is_popen_complete = 1;
       }
    }
   
   
    static uint64_t get_hfi_byte_cnt()
    {
    
      uint64_t hfi_byte_cnt=0.0;

      g_popen_complete_signal_action.sa_handler = geopm_popen_complete;
      sigemptyset(&g_popen_complete_signal_action.sa_mask);
      g_popen_complete_signal_action.sa_flags = 0;
      struct sigaction save_action;

      int err = sigaction(SIGCHLD, &g_popen_complete_signal_action, &save_action);

      if (!err) {

    	 FILE *opatools_fileptr = popen("opapmaquery 2>&1 | grep \"Xmit Data\" |  awk -F'[[:space:]]*|[(]'  '{print $7}'", "r");
	 if (opatools_fileptr) {
	    while (!g_is_popen_complete) {
	    }
	    g_is_popen_complete = 0;
      	    char opatools_dump[OPATOOLS_OUT_MAX_LEN];
            while (fgets(opatools_dump, OPATOOLS_OUT_MAX_LEN, opatools_fileptr) != NULL) {
               hfi_byte_cnt = std::stoll(opatools_dump)<<3; // bytes=8*flits
            }
            if(pclose(opatools_fileptr)==1) {
	        printf("Error!!! pclose() failed on opatools\n");
	    }
	 }
	 sigaction(SIGCHLD, &save_action, NULL);						 
      }

      return hfi_byte_cnt;
    }


    // Linear fit of line with two points corresponding
    // to idle and typical power numbers for the HFI
    static inline  double convert_bw_to_dynamic_hfi_power(double observed_bw_gbps)
    {
   	return ((observed_bw_gbps-HFI_MIN_BW_GBPS)*(HFI_MAX_POWER-HFI_IDLE_POWER)/
		(HFI_MAX_BW_GBPS-HFI_MIN_BW_GBPS)
	       ) + HFI_IDLE_POWER; 
    }	    


    static inline const double get_static_hfi_power()
    {
        return HFI_IDLE_POWER;
    }


    static FILE* hfi_report;
    static char hostname_arr[30];
    static double get_dynamic_hfi_power()
    {
	const uint64_t MAX_COUNTER_VAL = (-1L)>>1;

        static short  is_logfile_open=0;
        static uint32_t log_cnt=-1;

	struct geopm_time_s hfi_current_timer;
	static struct geopm_time_s hfi_previous_timer, hfi_start_timer;
        static double delta_time, hfi_dyn_power;

	static uint64_t hfi_previous_bytes=0x0; 
	static uint64_t hfi_current_bytes_arr[NW_LOG_REC_MAX], 
		        delta_bytes_arr[NW_LOG_REC_MAX]; 

	static double delta_time_arr[NW_LOG_REC_MAX],
	              timestamp_arr[NW_LOG_REC_MAX], 
	              avg_bandwidth_arr[NW_LOG_REC_MAX], 
		      hfi_dyn_power_arr[NW_LOG_REC_MAX]; 


	if(is_logfile_open==0) {
	   gethostname(hostname_arr,30);
           hfi_report = fopen(hostname_arr,"w");
           fprintf(hfi_report,"#TimeStamp, Inst_Bytes, Power, Delta Bytes, Delta Time, Bandwidth\n"); 
  	   geopm_time(&hfi_start_timer);	
  	   geopm_time(&hfi_previous_timer);	
           is_logfile_open=1;
        }

  	geopm_time(&hfi_current_timer);	
	delta_time = geopm_time_diff(&hfi_previous_timer, &hfi_current_timer);

	// should record be logged?
	if (delta_time >= 0.001) {

	   log_cnt++;

	   // record timestamps
           timestamp_arr[log_cnt] = geopm_time_diff(&hfi_start_timer, 
			   			&hfi_current_timer);
	   delta_time_arr[log_cnt] = delta_time;

	   // get and record HFI counter
	   hfi_current_bytes_arr[log_cnt] = get_hfi_byte_cnt();

	   // correct any HFI counter overflow errors
	   if (hfi_current_bytes_arr[log_cnt] < hfi_previous_bytes) { 
           	delta_bytes_arr[log_cnt] = MAX_COUNTER_VAL -
			                   hfi_previous_bytes +
					   hfi_current_bytes_arr[log_cnt];
	   } else {
		delta_bytes_arr[log_cnt] = hfi_current_bytes_arr[log_cnt] - 
					   hfi_previous_bytes;
	   }

	   // calculate and record bandwidth and HFI _dynamic_ power
	   avg_bandwidth_arr[log_cnt] = (double)delta_bytes_arr[log_cnt] / 
		                        delta_time_arr[log_cnt];
	   hfi_dyn_power = convert_bw_to_dynamic_hfi_power(avg_bandwidth_arr[log_cnt]) - 
		           HFI_IDLE_POWER;
	   hfi_dyn_power_arr[log_cnt] = hfi_dyn_power;

	   // bookkeeping for future iterations 
	   hfi_previous_timer = hfi_current_timer;
	   hfi_previous_bytes = hfi_current_bytes_arr[log_cnt];

	   // dump records on detecting full buffer
	   if(log_cnt == NW_LOG_REC_MAX-1) {
            	for (log_cnt=0; log_cnt<NW_LOG_REC_MAX; log_cnt++) {
            		fprintf(hfi_report,"%g, %lu, %g, %lu, %g, %g\n", 
	    	      		           timestamp_arr[log_cnt], 
	    	      		           hfi_current_bytes_arr[log_cnt], 
	    	      		           hfi_dyn_power_arr[log_cnt], 
	    	      		           delta_bytes_arr[log_cnt], 
	    	      		           delta_time_arr[log_cnt], 
	    	      		           avg_bandwidth_arr[log_cnt]);
	    	}
	    	log_cnt = 0;
	   }
	}
	return hfi_dyn_power;
    }

#endif /* HFI_POWER_ACCOUNTING_SUPPORT */


    bool GoverningDecider::update_policy(IRegion &curr_region, IPolicy &curr_policy)
    {
        static const double GUARD_BAND = 0.02;
        bool is_target_updated = false;
        const uint32_t region_id = curr_region.identifier();


        // If we have enough samples from the current region then update policy.
        if (curr_region.num_sample(0, GEOPM_TELEMETRY_TYPE_PKG_ENERGY) >= m_num_sample) {
            const int num_domain = curr_policy.num_domain();
            std::vector<double> limit(num_domain);
	    // SJ: "target" --> setting for the policy 
            std::vector<double> target(num_domain);
            std::vector<double> domain_dram_power(num_domain);
            // Get node limit for epoch set by tree decider
            curr_policy.target(GEOPM_REGION_ID_EPOCH, limit);
            // Get last policy target for the current region
            curr_policy.target(region_id, target);

            // Sum package and dram power over all domains to get total_power
	    
            double dram_power = 0.0;
            double limit_total = 0.0;


            for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                domain_dram_power[domain_idx] = curr_region.derivative(domain_idx, GEOPM_TELEMETRY_TYPE_DRAM_ENERGY);
                dram_power += domain_dram_power[domain_idx];
                limit_total += limit[domain_idx];
            }

#ifdef HFI_POWER_ACCOUNTING_SUPPORT
	    // Subtract HFI power from the total node power budget
	    // For now, we are assuming that there is only one HFI per node (a typical
	    // KNL+WFR,SKX+WFR setup). So, the HFI power isn't split across multiple domains.
	    
	    double hfi_power = get_static_hfi_power() + get_dynamic_hfi_power();
            limit_total -= hfi_power;

#endif /* HFI_POWER_ACCOUNTING_SUPPORT */


            double upper_limit = m_last_dram_power + (GUARD_BAND * limit_total);
            double lower_limit = m_last_dram_power - (GUARD_BAND * limit_total);

            // If we have enough energy samples to accurately
            // calculate power: derivative function did not return NaN.
            if (!std::isnan(dram_power)) {
                if (dram_power < lower_limit || dram_power > upper_limit) {
                    m_last_dram_power = dram_power;
                    for (int domain_idx = 0; domain_idx < num_domain; ++domain_idx) {
                        target[domain_idx] = limit[domain_idx] - domain_dram_power[domain_idx];
                    }
		    // set a new target value for region_id
                    curr_policy.update(region_id, target);
                    is_target_updated = true;
                }
                if (!curr_policy.is_converged(region_id)) {
                    if (is_target_updated) {
                        // Set to zero the number of times we were
                        // under budget since last in "converged"
                        // state (since policy is not currently in a
                        // converged state).
                        auto it = m_num_converged.lower_bound(region_id);
                        if (it != m_num_converged.end() && (*it).first == region_id) {
                            (*it).second = 0;
                        }
                        else {
                            it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 0));
                        }
                    }
                    else {
                        // If the region's policy is in a state of
                        // "unconvergence", but the node is currently
                        // under the target budget increment the
                        // number of samples under budget since last
                        // last convergence or last overage.
                        auto it = m_num_converged.lower_bound(region_id);
                        if (it != m_num_converged.end() && (*it).first == region_id) {
                            ++(*it).second;
                        }
                        else {
                            it = m_num_converged.insert(it, std::pair<uint64_t, unsigned>(region_id, 1));
                        }
                        if ((*it).second >= m_min_num_converged) {
                            // If we have observed m_min_num_converged
                            // samples in a row under budget set the
                            // region to the "converged" state and
                            // reset the counter.
                            curr_policy.is_converged(region_id, true);
                            (*it).second = 0;
                        }
                    }
                }
            }
        }

        return is_target_updated;
    }
}
