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

#include <assert.h>
#include <cstring>  //strcmp
#include <errno.h>
#include <float.h>  //DBL_MAX
#include <fstream>  //ifstream
#include <limits.h>  // INT_MAX
#include <sched.h>
#include <signal.h> //SIGALRM
#include <sstream>  //istringstream
#include <stdbool.h>
#include <stdint.h> //uint64_t
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h> //sysconf, _SC_PROCESSORS_ONLN
#include <time.h>
#include <mpi.h>

#include "geopm.h"
#include "geopm_time.h"
#include "synthetic_benchmark.hpp"

SyntheticBenchmarkConfig::SyntheticBenchmarkConfig()
{
    m_filename_static = "";
    m_num_iters = -1,
    m_max_iters = -1;
    m_min_iters = -1;
    m_cap_iters = INT_MAX;
    m_rank_norm = NULL;
    m_rank_iters = NULL;
    m_loadfactor_static = -1.0;
    m_rank_runtime = NULL;
    m_setcapIters = false,
    m_useRandomStatic = false,
    m_useReplayStatic = false,
    m_useStaticImbalance = false,
    m_enableRebalancing = false;
}

SyntheticBenchmarkConfig::~SyntheticBenchmarkConfig()
{
    delete [] m_rank_iters;
    delete [] m_rank_runtime;
    delete [] m_rank_norm;
}

void SyntheticBenchmarkConfig::initialize(int nranks)
{
    m_rank_iters = new int[nranks];
    m_rank_runtime = new double[nranks];
    m_rank_norm = new double[nranks];
    for (int i = 0; i < nranks; i++) {
        m_rank_iters[i] = 0;
        m_rank_runtime[i] = 0.0;
        m_rank_norm[i] = -1.0;
    }
}

void dumpConfiguration(int nranks)
{
    printf("---------- Configuration ----------\n");
    printf("Rebalancing (0=off, 1=on): %d\n", syntheticcfg.enableRebalancing());
    printf("MPI Ranks: %d\n", nranks);
    if (syntheticcfg.setcapIters()) {
        printf("Cap Iters/Rank: %d\n", syntheticcfg.cap_iters());
    }
}

void dumpSummary(int nranks, double elapsedTime)
{
    printf("\n---------- Summary ----------\n");
    if (syntheticcfg.useRandomStatic()) {
        printf("Static Load Imbalance: %.2f\n", syntheticcfg.loadfactor_static());
    }

    printf("Program Runtime: %.6f\n", elapsedTime);

    printf("Runtime Per Rank: ");
    for (int j = 0; j < nranks; j++) {
        printf("%.6f ", syntheticcfg.rank_runtime(j));
    }
    printf("\n");

    printf("Iterations per Rank: ");
    for (int j = 0; j < nranks; j++) {
        printf("%d ", syntheticcfg.rank_iters(j));
    }
    printf("\n");

    printf("WaitTime per Rank: ");
    for (int j = 0; j < nranks; j++) {
        printf("0.0 ");
    }
    printf("\n");
}

inline double do_work(int input)
{
    int i;
    double result = (double)input;

    for (i = 0; i < 100000; i++) {
        result += i*result;
    }

    return result;
}

void dumpRankAffinity(const char *rankid, pthread_t pid, int cid, const char *id)
{
    FILE *RankAffinityFile = fopen(RANK_AFFINITY_LOG, "a");
    if (RankAffinityFile != NULL) {
        fprintf(RankAffinityFile, "%2s %ld %2d %s\n", rankid, (long)pid, cid, id);
        fclose(RankAffinityFile);
    }
}

void printError(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n\n", msg);
}

struct MinMax setRandStaticImbalance(int nranks, int *rank_iters, int minI, int maxI)
{
    struct MinMax m = {100000, 0};
    for (int i = 0; i < nranks; i++) {
        rank_iters[i] = (rand() % (maxI+1-minI))+minI;
        if (rank_iters[i] > m.max) {
            m.max = rank_iters[i];
            m.maxIdx = i;
        }
        if (rank_iters[i] < m.min) {
            m.min = rank_iters[i];
            m.minIdx = i;
        }
    }

    // Set end bounds of iteration range.
    rank_iters[m.minIdx] = syntheticcfg.min_iters();
    rank_iters[m.maxIdx] = syntheticcfg.max_iters();
    m.min = rank_iters[m.minIdx];
    m.max = rank_iters[m.maxIdx];

    return m;
}

struct MinMax setReplayStaticImbalance(int nranks, int *rank_iters, string infile)
{
    std::ifstream file;
    string line;
    int numLines = 0;
    struct MinMax m = {100000, 0};

    file.open(infile.c_str());
    if (!file.good()) {
        fprintf(stderr, "ERROR: File %s does not exist!\n", infile.c_str());
        exit(EXIT_FAILURE);
    }

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        iss >> rank_iters[numLines];
        if (rank_iters[numLines] > m.max) {
            m.max = rank_iters[numLines];
        }
        if (rank_iters[numLines] < m.min) {
            m.min = rank_iters[numLines];
        }
        ++numLines;
        if (numLines > nranks) {
            printf("Warning: Extra entries in %s. Only using first %d lines.\n",
                   infile.c_str(), nranks);
            break;
        }
    }

    if (numLines < nranks) {
        printf("Error: Not enough entries in %s, numLines = %d.\n",
               infile.c_str(), numLines);
        exit(EXIT_FAILURE);
    }

    return m;
}

void dumpRankItersReplay(int nranks, int *rankIters)
{
    FILE *RankItersConfigFile = fopen(RANK_ITERATIONS_CONFIG, "w");
    if (RankItersConfigFile != NULL) {
        for (int i = 0; i < nranks; i++) {
            fprintf(RankItersConfigFile, "%d\n", rankIters[i]);
        }
        fclose(RankItersConfigFile);
    }
}

void dumpRankIters(int nranks, int *rankIters)
{
    FILE *RankItersLogFile = fopen(RANK_ITERATIONS_LOG, "w");
    if (RankItersLogFile != NULL) {
        fprintf(RankItersLogFile, "omp_tid, niters\n");
        for (int i = 0; i < nranks; i++) {
            fprintf(RankItersLogFile, "%d, %d\n", i, rankIters[i]);
        }
        fclose(RankItersLogFile);
    }
}

void dumpRankRuntime(int nranks, double *rank_runtime)
{
    FILE *RankRuntimeLogFile = fopen(RANK_RUNTIME_LOG, "w");
    if (RankRuntimeLogFile != NULL) {
        fprintf(RankRuntimeLogFile, "omp_tid, runtime\n");
        for (int i = 0; i < nranks; i++) {
            fprintf(RankRuntimeLogFile, "%d, %.4f\n", i, rank_runtime[i]);
        }
        fclose(RankRuntimeLogFile);
    }
}

int round_int(double d)
{
    return (d > 0.0) ? (d + 0.5) : (d - 0.5);
}

void initStaticImbalance(int nranks)
{
    struct MinMax m = {100000, 0};

    // Generate random static imbalance.
    if (syntheticcfg.useRandomStatic() && !(syntheticcfg.useReplayStatic())) {
        printf("Static Imbalance Generator: Random, %.2f\n",
               syntheticcfg.loadfactor_static());

        // If load imbalance factor is 0, each rank will have an iteration
        // count equal to value specified on command line with -niter.
        if (syntheticcfg.loadfactor_static() == 0.0) {
            printf("Iterations per Rank: %d\n", syntheticcfg.num_iters());
            for (int i = 0; i < nranks; i++) {
                syntheticcfg.rank_iters(i, syntheticcfg.num_iters());
            }
            m.min = syntheticcfg.num_iters();
            m.max = syntheticcfg.num_iters();
        }
        // If load imbalance factor is greater than 0, each rank will have
        // an iteration count randomly generated between value specified on
        // command line with -niter and this value multiplied by 1 plus load
        // factor, inclusive.
        else {
            syntheticcfg.max_iters(round_int(syntheticcfg.num_iters()*(1+(syntheticcfg.loadfactor_static()/2))));
            syntheticcfg.min_iters(round_int(syntheticcfg.num_iters()*(1-(syntheticcfg.loadfactor_static()/2))));
            m = setRandStaticImbalance(nranks, syntheticcfg.rank_iters(), syntheticcfg.min_iters(), syntheticcfg.max_iters());
        }

        // Save iterations into config file for replay.
        dumpRankItersReplay(nranks, syntheticcfg.rank_iters());
    }
    // Replay static imbalance from file.
    else if (!(syntheticcfg.useRandomStatic()) && syntheticcfg.useReplayStatic()) {
        printf("Static Imbalance Generator: Replay, %s\n",
               syntheticcfg.filename_static().c_str());
        m = setReplayStaticImbalance(nranks, syntheticcfg.rank_iters(), syntheticcfg.filename_static());
        syntheticcfg.max_iters((int)m.max);
        syntheticcfg.min_iters((int)m.min);
    }

    if (syntheticcfg.loadfactor_static() != 0.0) {
        printf("Iterations per Rank Range: %d-%d\n", (int)m.min, (int)m.max);
    }
}

void synthetic_benchmark_main(int nranks, int rank)
{
    uint64_t region_id[2];
    struct geopm_time_s start, curr;
    double x = 0.0;
    double tStart = 0.0, tEnd = 0.0, startProg = 0.0, endProg = 0.0;
    double* sub_rank_runtime = NULL;
    sub_rank_runtime = new double;

    GET_TIME(startProg);

    syntheticcfg.initialize(nranks);

    // Only 1 rank generates array of iterations
    if (rank == MASTER) {
        dumpConfiguration(nranks);

        if (syntheticcfg.useStaticImbalance()) {
            initStaticImbalance(nranks);
            printf("\n");
            dumpRankIters(nranks, syntheticcfg.rank_iters());
            for (int i = 0; i < nranks; i++)
                syntheticcfg.rank_norm(i, 1/(double)syntheticcfg.rank_iters(i));
        }

        FILE *RankAffinityLog = fopen(RANK_AFFINITY_LOG, "w");
        if (RankAffinityLog != NULL) {
            fprintf(RankAffinityLog, "rankID pthread_pid cpu_cid name\n");
            fclose(RankAffinityLog);
        }
    }
    // Broadcast iterations to every rank
    MPI_Bcast(syntheticcfg.rank_iters(), nranks, MPI_INT, MASTER, MPI_COMM_WORLD);
    // Broadcast norm to every rank
    MPI_Bcast(syntheticcfg.rank_norm(), nranks, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

    pthread_t my_pid = pthread_self();
    char buf[256];
    sprintf(buf, "%d", rank);
    dumpRankAffinity(buf, my_pid, sched_getcpu(), "Workload");

    geopm_prof_region("loop_one", GEOPM_REGION_HINT_UNKNOWN, &region_id[0]);
    geopm_prof_enter(region_id[0]);
    geopm_time(&start);

    GET_TIME(tStart);
    for (int i = 0; i < syntheticcfg.rank_iters(rank); i++) {
        // Check if user wants to terminate early and not run all iterations.
        if (syntheticcfg.setcapIters()) {
            if (i == syntheticcfg.cap_iters()) {
                break;
            }
        }

        x += do_work(i);
        geopm_prof_progress(region_id[0], i*syntheticcfg.rank_norm(rank));
    }
    GET_TIME(tEnd);
    fprintf(stderr, "%.2fs: Rank %d finished\n", tEnd-tStart, rank);

    geopm_time(&curr);
    geopm_prof_exit(region_id[0]);

    *sub_rank_runtime = tEnd-tStart;

    MPI_Allgather(sub_rank_runtime, 1, MPI_DOUBLE, syntheticcfg.rank_runtime(), 1, MPI_DOUBLE, MPI_COMM_WORLD);

    GET_TIME(endProg);

    // Master rank does reporting
    if (rank == MASTER) {
        dumpRankRuntime(nranks, syntheticcfg.rank_runtime());
        dumpSummary(nranks, endProg-startProg);
    }

}

int main(int argc, char **argv)
{
    const char *usage = "    %s [--help]\n"
                        "    (-r static_rand | -c static_config)\n"
                        "    [-i num_iters] [-p] [-m max_iterations]\n"
                        "\n"
                        "    --help\n"
                        "           Print brief summary of the command line usage information, then\n"
                        "           exit.\n"
                        "   -r imbalance_factor\n"
                        "           Induce random static imbalance based on load imbalance factor.\n"
                        "   -c static_config\n"
                        "           Induce static imbalance based on configuration file. If -c is\n"
                        "           specified, then -r should not be specified. If both are\n"
                        "           specified, then -r is ignored.\n"
                        "   -i num_iters\n"
                        "           Minimum number of iterations per rank. Used in conjunction\n"
                        "           with -r static_rand.\n"
                        "   -p\n"
                        "           Enable per-core rebalancing algorithm.\n"
                        "   -m max_iterations\n"
                        "           Terminate ranks prematurely based on number of iterations.\n"
                        "\n";
    int nranks, rank;
    int opt;

    if (argc > 1 && (
            strncmp(argv[1], "--help", strlen("--help")) == 0 ||
            strncmp(argv[1], "-h", strlen("-h")) == 0)) {
        printf(usage, argv[0]);
        return 0;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nranks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while ((opt = getopt(argc, argv, "c:r:i:m:p")) != -1) {
        switch (opt) {
            case 'c':
                syntheticcfg.useStaticImbalance(true);
                syntheticcfg.useReplayStatic(true);
                syntheticcfg.filename_static(optarg);
                break;
            case 'r':
                syntheticcfg.useStaticImbalance(true);
                syntheticcfg.useRandomStatic(true);
                syntheticcfg.loadfactor_static(atof(optarg));
                srand(time(NULL));
                break;
            case 'i':
                syntheticcfg.num_iters(atoi(optarg));
                break;
            case 'm':
                syntheticcfg.cap_iters(atoi(optarg));
                syntheticcfg.setcapIters(true);
                break;
            case 'p':
                syntheticcfg.enableRebalancing(true);
                break;
            default:
                fprintf(stderr, "Error; unknown parameter \"%c\"\n", opt);
                fprintf(stderr, usage, argv[0]);
                return -1;
                break;
        }
    }
    //-----------------------------------------------------
    // Check that only 1 type of generator is selected for
    // either imbalance type, e.g., cannot inject static
    // imbalance via randomness AND file.
    //-----------------------------------------------------
    if (syntheticcfg.useReplayStatic() && syntheticcfg.useRandomStatic()) {
        printError("Must set only one option for imbalance generator: -c OR -r");
        printf(usage, argv[0]);
        return -1;
    }
    //-----------------------------------------------------
    //-----------------------------------------------------
    // Check that number of iterations is specified if want
    // to replay dynamic imbalance, inject random dynamic
    // imbalance, or inject random static imbalance.
    //-----------------------------------------------------
    if (syntheticcfg.useRandomStatic() && syntheticcfg.num_iters() == -1) {
        printError("Must set number of iterations (-i) if injecting "
                   "random static imbalance.");
        printf(usage, argv[0]);
        return -1;
    }
    //-----------------------------------------------------

    synthetic_benchmark_main(nranks, rank);

    MPI_Finalize();

    return 0;
}
