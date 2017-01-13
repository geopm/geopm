
    void Platform::tdp_limit(double percentage) const
    {
        //Get the TDP for each socket and set its power limit to match
        int packages = m_imp->num_package();
        double tdp = m_imp->package_tdp();
        double pkg_lim = tdp * (percentage * 0.01);
        for (int i = 0; i < packages; i++) {
            m_imp->write_control(m_imp->control_domain(GEOPM_CONTROL_TYPE_POWER), i,  GEOPM_TELEMETRY_TYPE_PKG_ENERGY, pkg_lim);
        }
    }

    void Platform::manual_frequency(int frequency, int num_cpu_max_perf, int affinity) const
    {
        //Set the frequency for each cpu
        int64_t freq_perc;
        bool small = false;
        int num_logical_cpus = m_imp->num_logical_cpu();
        int num_real_cpus = m_imp->num_hw_cpu();
        int packages = m_imp->num_package();
        int num_cpus_per_package = num_real_cpus / packages;
        int num_small_cores_per_package = num_cpus_per_package - (num_cpu_max_perf / packages);

        if (num_cpu_max_perf >= num_real_cpus) {
            throw Exception("requested number of max perf cpus is greater than controllable number of frequency domains on the platform",
                            GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
        }

        for (int i = 0; i < num_logical_cpus; i++) {
            int real_cpu = i % num_real_cpus;
            if (affinity == GEOPM_POLICY_AFFINITY_SCATTER && num_cpu_max_perf > 0) {
                if ((real_cpu % num_cpus_per_package) < num_small_cores_per_package) {
                    small = true;
                }
            }
            else if (affinity == GEOPM_POLICY_AFFINITY_COMPACT && num_cpu_max_perf > 0) {
                if (real_cpu < (num_real_cpus - num_cpu_max_perf)) {
                    small = true;
                }
            }
            else {
                small = true;
            }
            if (small) {
                freq_perc = ((int64_t)(frequency * 0.01) << 8) & 0xffff;
                m_imp->msr_write(GEOPM_DOMAIN_CPU, i, "IA32_PERF_CTL", freq_perc & 0xffff);
            }
            small = false;
        }
    }
