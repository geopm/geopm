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
        std::string filenameStatic;
        int         nIters,
                    maxIters,
                    minIters,
                    capIters,
                    *myIters;
        double      loadFactorStatic,
                    *rankRuntime,
                    *norm,
                    *waitLength,
                    *g_rankRuntime;
        bool        setcapIters,
                    useRandomStatic,
                    useReplayStatic,
                    useStaticImbalance,
                    enableRebalancing;
    public:
        SyntheticBenchmarkConfig();
        ~SyntheticBenchmarkConfig();

        std::string get_filenameStatic()
        {
            return filenameStatic;
        }
        int get_nIters()
        {
            return nIters;
        }
        int get_capIters()
        {
            return capIters;
        }
        int get_maxIters()
        {
            return maxIters;
        }
        int get_minIters()
        {
            return minIters;
        }
        int getIdx_myIters(int i)
        {
            return myIters[i];
        }
        int* get_myIters()
        {
            return myIters;
        }
        double get_loadFactorStatic()
        {
            return loadFactorStatic;
        }
        double getIdx_waitLength(int i)
        {
            return waitLength[i];
        }
        double getIdx_rankRuntime(int i)
        {
            return rankRuntime[i];
        }
        double getIdx_g_rankRuntime(int i)
        {
            return g_rankRuntime[i];
        }
        double getIdx_norm(int i)
        {
            return norm[i];
        }
        double* get_rankRuntime()
        {
            return rankRuntime;
        }
        double* get_g_rankRuntime()
        {
            return g_rankRuntime;
        }
        double* get_norm()
        {
            return norm;
        }
        double* get_waitLength()
        {
            return waitLength;
        }
        bool get_setcapIters()
        {
            return setcapIters;
        }
        bool get_useRandomStatic()
        {
            return useRandomStatic;
        }
        bool get_useReplayStatic()
        {
            return useReplayStatic;
        }
        bool get_useStaticImbalance()
        {
            return useStaticImbalance;
        }
        bool get_enableRebalancing()
        {
            return enableRebalancing;
        }

        void set_filenameStatic(std::string s)
        {
            filenameStatic = s;
        }
        void set_nIters(int i)
        {
            nIters = i;
        }
        void set_capIters(int i)
        {
            capIters = i;
        }
        void set_loadFactorStatic(double val)
        {
            loadFactorStatic = val;
        }
        void set_setcapIters(bool b)
        {
            setcapIters = b;
        }
        void set_useRandomStatic(bool b)
        {
            useRandomStatic = b;
        }
        void set_useReplayStatic(bool b)
        {
            useReplayStatic = b;
        }
        void set_useStaticImbalance(bool b)
        {
            useStaticImbalance = b;
        }
        void set_enableRebalancing(bool b)
        {
            enableRebalancing = b;
        }
        void set_maxIters(int i)
        {
            maxIters = i;
        }
        void set_minIters(int i)
        {
            minIters = i;
        }
        void setIdx_myIters(int i, int val)
        {
            myIters[i] = val;
        }
        void setIdx_norm(int i, double val)
        {
            norm[i] = val;
        }
        void setIdx_rankRuntime(int i, double val)
        {
            rankRuntime[i] = val;
        }
        void setIdx_waitLength(int i, double val)
        {
            waitLength[i] = val;
        }

        void init_config_arrays(int nranks);
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
                                     int *myIters,
                                     int min,
                                     int max);
struct MinMax setReplayStaticImbalance(int nranks,
                                       int *myIters,
                                       string input);
void dumpRankItersReplay(int nranks,
                         int *myIters);
void dumpRankIters(int nranks,
                   int *myIters);
void dumpRankRuntime(int nranks,
                     double *myRuntime);
void injectStaticImbalance();

#endif
