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

#ifndef SYNTHETIC_BENCHMARK_HPP_INCLUDE
#define SYNTHETIC_BENCHMARK_HPP_INCLUDE

#include <string>
using std::string;

#define GET_TIME(now) { \
    struct timeval t; \
    gettimeofday(&t, NULL); \
    now = t.tv_sec + t.tv_usec/1000000.0; \
}

#define RANK_AFFINITY_LOG      "rank_affinity.log"
#define RANK_RUNTIME_LOG       "runtime_per_rank.log"
#define RANK_ITERATIONS_LOG    "iterations_per_rank.log"
#define RANK_ITERATIONS_CONFIG "iterations_per_rank.config"

#define MASTER 0

struct MinMax {
    double min;
    double max;
    int minIdx;
    int maxIdx;
};

class SyntheticBenchmarkConfig
{
    private:
        std::string m_filename_static;
        int         m_num_iters,
                    m_max_iters,
                    m_min_iters,
                    m_cap_iters,
                    *m_rank_iters;
        double      m_loadfactor_static,
                    *m_rank_runtime,
                    *m_rank_norm;
        bool        m_setcapIters,
                    m_useRandomStatic,
                    m_useReplayStatic,
                    m_useStaticImbalance,
                    m_enableRebalancing;
    public:
        SyntheticBenchmarkConfig();
        ~SyntheticBenchmarkConfig();

        std::string filename_static(void) const
        {
            return m_filename_static;
        }
        int num_iters(void) const
        {
            return m_num_iters;
        }
        int cap_iters(void) const
        {
            return m_cap_iters;
        }
        int max_iters(void) const
        {
            return m_max_iters;
        }
        int min_iters(void) const
        {
            return m_min_iters;
        }
        int rank_iters(int i)
        {
            return m_rank_iters[i];
        }
        int* rank_iters(void) const
        {
            return m_rank_iters;
        }
        double loadfactor_static(void) const
        {
            return m_loadfactor_static;
        }
        double rank_runtime(int i)
        {
            return m_rank_runtime[i];
        }
        double rank_norm(int i)
        {
            return m_rank_norm[i];
        }
        double* rank_runtime(void) const
        {
            return m_rank_runtime;
        }
        double* rank_norm(void) const
        {
            return m_rank_norm;
        }
        bool setcapIters(void) const
        {
            return m_setcapIters;
        }
        bool useRandomStatic(void) const
        {
            return m_useRandomStatic;
        }
        bool useReplayStatic(void) const
        {
            return m_useReplayStatic;
        }
        bool useStaticImbalance(void) const
        {
            return m_useStaticImbalance;
        }
        bool enableRebalancing(void) const
        {
            return m_enableRebalancing;
        }

        void filename_static(std::string s)
        {
            m_filename_static = s;
        }
        void num_iters(int i)
        {
            m_num_iters = i;
        }
        void cap_iters(int i)
        {
            m_cap_iters = i;
        }
        void loadfactor_static(double val)
        {
            m_loadfactor_static = val;
        }
        void setcapIters(bool b)
        {
            m_setcapIters = b;
        }
        void useRandomStatic(bool b)
        {
            m_useRandomStatic = b;
        }
        void useReplayStatic(bool b)
        {
            m_useReplayStatic = b;
        }
        void useStaticImbalance(bool b)
        {
            m_useStaticImbalance = b;
        }
        void enableRebalancing(bool b)
        {
            m_enableRebalancing = b;
        }
        void max_iters(int i)
        {
            m_max_iters = i;
        }
        void min_iters(int i)
        {
            m_min_iters = i;
        }
        void rank_iters(int i, int val)
        {
            m_rank_iters[i] = val;
        }
        void rank_norm(int i, double val)
        {
            m_rank_norm[i] = val;
        }

        void initialize(int nranks);
};

SyntheticBenchmarkConfig syntheticcfg;

inline double do_work(int input);
void dumpRankAffinity(const char *omp_tid,
                      pthread_t pid,
                      int cid,
                      const char *id);
void printUsage(void);
void printError(const char *c);
struct MinMax setRandStaticImbalance(int nranks,
                                     int *rank_iters,
                                     int min,
                                     int max);
struct MinMax setReplayStaticImbalance(int nranks,
                                       int *rank_iters,
                                       string input);
void dumpRankItersReplay(int nranks,
                         int *rank_iters);
void dumpRankIters(int nranks,
                   int *rank_iters);
void dumpRankRuntime(int nranks,
                     double *myRuntime);
void injectStaticImbalance();

#endif
