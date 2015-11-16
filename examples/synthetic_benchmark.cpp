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

#include "geopm.h"
#include "synthetic_benchmark.hpp"

SyntheticBenchmarkConfig::SyntheticBenchmarkConfig()
{
    filenameStatic = "";
    nIters = -1,
    maxIters = -1;
    minIters = -1;
    capIters = INT_MAX;
    myIters = NULL;
    loadFactorStatic = -1.0;
    rankRuntime = NULL;
    g_rankRuntime = NULL;
    setcapIters = false,
    useRandomStatic = false,
    useReplayStatic = false,
    useStaticImbalance = false,
    enableRebalancing = false;
}

SyntheticBenchmarkConfig::~SyntheticBenchmarkConfig()
{
    delete [] myIters;
    delete [] rankRuntime;
    delete [] norm;
    delete [] waitLength;
    delete [] g_rankRuntime;
}

void SyntheticBenchmarkConfig::init_config_arrays(int nranks)
{
    myIters = new int[nranks+1];
    rankRuntime = new double[nranks+1];
    g_rankRuntime = new double[nranks+1];
    waitLength = new double[nranks+1];
    norm = new double[nranks+1];
    for (int i = 0; i < nranks; i++) {
        myIters[i] = 0;
        rankRuntime[i] = 0.0;
        g_rankRuntime[i] = 0.0;
        waitLength[i] = -1.0;
        norm[i] = -1.0;
    }
}

void dumpConfiguration(int nranks)
{
    printf("---------- Configuration ----------\n");
    printf("Rebalancing (0=off, 1=on): %d\n", syntheticcfg.get_enableRebalancing());
    printf("MPI Ranks: %d\n", nranks);
    if (syntheticcfg.get_setcapIters()) {
        printf("Cap Iters/Rank: %d\n", syntheticcfg.get_capIters());
    }
}

void dumpSummary(int nranks, double elapsedTime)
{
    printf("\n---------- Summary ----------\n");
    if (syntheticcfg.get_useRandomStatic()) {
        printf("Static Load Imbalance: %.2f\n", syntheticcfg.get_loadFactorStatic());
    }

    printf("Program Runtime: %.6f\n", elapsedTime);

    printf("Runtime Per Rank: ");
    for (int j = 0; j < nranks; j++) {
        printf("%.6f ", syntheticcfg.getIdx_g_rankRuntime(j));
    }
    printf("\n");

    printf("Iterations per Rank: ");
    for (int j = 0; j < nranks; j++) {
        printf("%d ", syntheticcfg.getIdx_myIters(j));
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
    fprintf(RankAffinityFile, "%2s %ld %2d %s\n", rankid, pid, cid, id);
    fclose(RankAffinityFile);
}

void printError(const char *msg)
{
    fprintf(stderr, "ERROR: %s\n\n", msg);
}

struct MinMax setRandStaticImbalance(int nranks, int *myIters, int minI, int maxI)
{
    struct MinMax m = {100000, 0};
    for (int i = 0; i < nranks; i++) {
        myIters[i] = (rand() % (maxI+1-minI))+minI;
        if (myIters[i] > m.max) {
            m.max = myIters[i];
            m.maxIdx = i;
        }
        if (myIters[i] < m.min) {
            m.min = myIters[i];
            m.minIdx = i;
        }
    }

    // Set end bounds of iteration range.
    myIters[m.minIdx] = syntheticcfg.get_minIters();
    myIters[m.maxIdx] = syntheticcfg.get_maxIters();
    m.min = myIters[m.minIdx];
    m.max = myIters[m.maxIdx];

    return m;
}

struct MinMax setReplayStaticImbalance(int nranks, int *myIters, string infile)
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
        iss >> myIters[numLines];
        if (myIters[numLines] > m.max) {
            m.max = myIters[numLines];
        }
        if (myIters[numLines] < m.min) {
            m.min = myIters[numLines];
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
    for (int i = 0; i < nranks; i++) {
        fprintf(RankItersConfigFile, "%d\n", rankIters[i]);
    }
    fclose(RankItersConfigFile);
}

void dumpRankIters(int nranks, int *rankIters)
{
    FILE *RankItersLogFile = fopen(RANK_ITERATIONS_LOG, "w");
    fprintf(RankItersLogFile, "omp_tid, niters\n");
    for (int i = 0; i < nranks; i++) {
        fprintf(RankItersLogFile, "%d, %d\n", i, rankIters[i]);
    }
    fclose(RankItersLogFile);
}

void dumpRankRuntime(int nranks, double *rankRuntime)
{
    FILE *RankRuntimeLogFile = fopen(RANK_RUNTIME_LOG, "w");
    fprintf(RankRuntimeLogFile, "omp_tid, runtime\n");
    for (int i = 0; i < nranks; i++) {
        fprintf(RankRuntimeLogFile, "%d, %.4f\n", i, rankRuntime[i]);
    }
    fclose(RankRuntimeLogFile);
}

int round_int(double d)
{
    return (d > 0.0) ? (d + 0.5) : (d - 0.5);
}

void initStaticImbalance(int nranks)
{
    struct MinMax m;

    // Generate random static imbalance.
    if (syntheticcfg.get_useRandomStatic() && !(syntheticcfg.get_useReplayStatic())) {
        printf("Static Imbalance Generator: Random, %.2f\n",
               syntheticcfg.get_loadFactorStatic());

        // If load imbalance factor is 0, each rank will have an iteration
        // count equal to value specified on command line with -niter.
        if (syntheticcfg.get_loadFactorStatic() == 0.0) {
            printf("Iterations per Rank: %d\n", syntheticcfg.get_nIters());
            for (int i = 0; i < nranks; i++) {
                syntheticcfg.setIdx_myIters(i, syntheticcfg.get_nIters());
            }
            m.min = syntheticcfg.get_nIters();
            m.max = syntheticcfg.get_nIters();
        }
        // If load imbalance factor is greater than 0, each rank will have
        // an iteration count randomly generated between value specified on
        // command line with -niter and this value multiplied by 1 plus load
        // factor, inclusive.
        else {
            syntheticcfg.set_maxIters(round_int(syntheticcfg.get_nIters()*(1+(syntheticcfg.get_loadFactorStatic()/2))));
            syntheticcfg.set_minIters(round_int(syntheticcfg.get_nIters()*(1-(syntheticcfg.get_loadFactorStatic()/2))));
            m = setRandStaticImbalance(nranks, syntheticcfg.get_myIters(),
                                       syntheticcfg.get_minIters(), syntheticcfg.get_maxIters());
        }

        // Save iterations into config file for replay.
        dumpRankItersReplay(nranks, syntheticcfg.get_myIters());
    }
    // Replay static imbalance from file.
    else if (!(syntheticcfg.get_useRandomStatic()) && syntheticcfg.get_useReplayStatic()) {
        printf("Static Imbalance Generator: Replay, %s\n",
               syntheticcfg.get_filenameStatic().c_str());
        m = setReplayStaticImbalance(nranks, syntheticcfg.get_myIters(),
                                     syntheticcfg.get_filenameStatic());
        syntheticcfg.set_maxIters((int)m.max);
        syntheticcfg.set_minIters((int)m.min);
    }

    if (syntheticcfg.get_loadFactorStatic() != 0.0) {
        printf("Iterations per Rank Range: %d-%d\n", (int)m.min, (int)m.max);
    }
}

void synthetic_benchmark_main(int nranks, int rank)
{
    struct geopm_prof_c *prof;
    uint64_t region_id[2];
    struct geopm_time_s start, curr;
    double x = 0.0;
    double tStart = 0.0, tEnd = 0.0, startProg = 0.0, endProg = 0.0;

    GET_TIME(startProg);

    syntheticcfg.init_config_arrays(nranks);

    // Only 1 rank generates array of iterations
    if (rank == MASTER) {
        dumpConfiguration(nranks);

        if (syntheticcfg.get_useStaticImbalance()) {
            initStaticImbalance(nranks);
            printf("\n");
            dumpRankIters(nranks, syntheticcfg.get_myIters());
            for (int i = 0; i < nranks; i++)
                syntheticcfg.setIdx_norm(i, 1/(double)syntheticcfg.getIdx_myIters(i));
        }

        FILE *RankAffinityLog = fopen(RANK_AFFINITY_LOG, "w");
        fprintf(RankAffinityLog, "rankID pthread_pid cpu_cid name\n");
        fclose(RankAffinityLog);
    }
    // Broadcast iterations to every rank
    MPI_Bcast(syntheticcfg.get_myIters(), nranks, MPI_INT, MASTER, MPI_COMM_WORLD);
    // Broadcast norm to every rank
    MPI_Bcast(syntheticcfg.get_norm(), nranks, MPI_DOUBLE, MASTER, MPI_COMM_WORLD);

    pthread_t my_pid = pthread_self();
    char buf[256];
    sprintf(buf, "%d", rank);
    dumpRankAffinity(buf, my_pid, sched_getcpu(), "Workload");

    geopm_prof_create("geopm_synthetic_benchmark", 4096, "/geopm_ctl_single", MPI_COMM_WORLD, &prof);
    geopm_prof_region(prof, "loop_one", GEOPM_POLICY_HINT_UNKNOWN, &region_id[0]);
    geopm_prof_enter(prof, region_id[0]);
    geopm_time(&start);

    GET_TIME(tStart);
    for (int i = 0; i < syntheticcfg.getIdx_myIters(rank); i++) {
        // Check if user wants to terminate rank early
        // and not execute all assigned iterations.
        if (syntheticcfg.get_setcapIters()) {
            if (i == syntheticcfg.get_capIters()) {
                break;
            }
        }

        x += do_work(i);
        geopm_prof_progress(prof, region_id[0], i*syntheticcfg.getIdx_norm(rank));
    }
    GET_TIME(tEnd);
    fprintf(stderr, "%.2fs: Rank %d finished\n", tEnd-tStart, rank);

    geopm_time(&curr);
    geopm_prof_exit(prof, region_id[0]);

    syntheticcfg.setIdx_rankRuntime(rank, tEnd-tStart);

    //TODO: More efficient implementation is MPI_Allgather
    MPI_Allreduce(syntheticcfg.get_rankRuntime(),
                  syntheticcfg.get_g_rankRuntime(),
                  nranks,
                  MPI_DOUBLE,
                  MPI_SUM,
                  MPI_COMM_WORLD);

    GET_TIME(endProg);

    // Master rank does reporting
    if (rank == MASTER) {
        dumpRankRuntime(nranks, syntheticcfg.get_g_rankRuntime());
        dumpSummary(nranks, endProg-startProg);
    }

    geopm_prof_print(prof, "geopm_synthetic_benchmark.log", 0);
    geopm_prof_destroy(prof);
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
                syntheticcfg.set_useStaticImbalance(true);
                syntheticcfg.set_useReplayStatic(true);
                syntheticcfg.set_filenameStatic(optarg);
                break;
            case 'r':
                syntheticcfg.set_useStaticImbalance(true);
                syntheticcfg.set_useRandomStatic(true);
                syntheticcfg.set_loadFactorStatic(atof(optarg));
                srand(time(NULL));
                break;
            case 'i':
                syntheticcfg.set_nIters(atoi(optarg));
                break;
            case 'm':
                syntheticcfg.set_capIters(atoi(optarg));
                syntheticcfg.set_setcapIters(true);
                break;
            case 'p':
                syntheticcfg.set_enableRebalancing(true);
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
    if (syntheticcfg.get_useReplayStatic() && syntheticcfg.get_useRandomStatic()) {
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
    if (syntheticcfg.get_useRandomStatic() && syntheticcfg.get_nIters() == -1) {
        printError("Must set number of iterations (-i) if injecting "
                   "random static imbalance.");
        printf(usage, argv[0]);
        return -1;
    }
    //-----------------------------------------------------

    synthetic_benchmark_main(nranks, rank);

    MPI_Finalize();
}
