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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <omp.h>
#include <signal.h>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <set>
#include <numeric>
#include <exception>

#include "geopm_time.h"

/// structure definitions
struct msr_batch_op_s {
    uint16_t cpu;      /// @brief In: CPU to execute {rd/wr}msr ins.
    uint16_t isrdmsr;  /// @brief In: 0=wrmsr, non-zero=rdmsr
    int32_t err;       /// @brief Out: Error code from operation
    uint32_t msr;      /// @brief In: MSR Address to perform op
    uint64_t msrdata;  /// @brief In/Out: Input/Result to/from operation
    uint64_t wmask;    /// @brief Out: Write mask applied to wrmsr
};

struct msr_batch_array_s {
    uint32_t numops;                /// @brief In: # of operations in ops array
    struct msr_batch_op_s *ops;     /// @brief In: Array[numops] of operations
};

/// function declarations
extern "C" {
int dgemm_(char *transa, char *transb, int *m, int *n, int *k,
           double *alpha, double *a, int *lda, double *b, int *ldb,
           double *beta, double *c, int *ldc);
}
void *dgemm_thread(void *args);
void *sampling_thread(void *args);
long num_cpu(void);                 /// @brief Return: number of CPUs available
double cpu_freq_sticker(void);
double cpu_freq_min(void);
void spin(double delay);
void print_usage(void);

/// enum definitions
enum batch_op_e {
    BATCH_WRITE_1,
    BATCH_WRITE_2,
    BATCH_WRITE_3,
    BATCH_WRITE_4,
    BATCH_READ,
    BATCH_OPS,
};

/// macro definitions
#define SAMPLE_ARG_STR          "--sample"
#define DGEMM_ARG_STR           "--dgemm"
#define IA_32_PERF_STATUS_MSR   0x198
#define IA_32_PERF_CTL_MSR      0x199
#define IA_32_PERF_MASK         0xFF00

#define BATCH_DEVICE_PATH       "/dev/cpu/msr_batch"
#define X86_IOC_MSR_BATCH       _IOWR('c', 0xA2, struct msr_batch_array_s)

#define CPU_INFO_PATH           "/proc/cpuinfo"
#define CPU_FREQ_MIN_PATH       "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq"
#define CPU_FREQ_MAX_PATH       "/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"

#define FREQ_SAMPLE_DELAY       1e-3

#define NUM_WRITE_BATCHES       BATCH_READ
#define MAX_SAMPLES             1000
#define NUM_TESTS               1000 // TODO command line configurable

#ifndef MATRIX_SIZE
#define MATRIX_SIZE 10240ULL
#endif
#ifndef PAD_SIZE
#define PAD_SIZE 128ULL
#endif
#ifndef NUM_DGEMM_REP
#define NUM_DGEMM_REP 400
#endif
#ifndef NAME_MAX
#define NAME_MAX 512
#endif

#define MAX_NUM_SOCKET 16

/// function definitions
long num_cpu(void) {
    return sysconf(_SC_NPROCESSORS_ONLN);
}

double cpu_freq_sticker(void)
{
    double result = NAN;
    const std::string key = "model name\t:";
    std::ifstream cpuinfo_file(CPU_INFO_PATH);
    if (!cpuinfo_file.is_open()) {
        exit(-1);
    }
    while (isnan(result) && cpuinfo_file.good()) {
        std::string line;
        getline(cpuinfo_file, line);
        if (line.find(key) != std::string::npos) {
            size_t at_pos = line.find("@");
            size_t ghz_pos = line.find("GHz");
            if (at_pos != std::string::npos &&
                    ghz_pos != std::string::npos) {
                try {
                    result = 1e9 * std::stod(line.substr(at_pos + 1, ghz_pos - at_pos));
                }
                catch (std::invalid_argument) {

                }
            }
        }
    }
    cpuinfo_file.close();
    if (isnan(result)) {
        exit(-1);
    }
    return result;
}

double cpu_freq_min(void)
{
    double result = NAN;
    std::ifstream freq_file(CPU_FREQ_MIN_PATH);
    if (freq_file.is_open()) {
        std::string line;
        getline(freq_file, line);
        try {
            result = 1e4 * std::stod(line);
        }
        catch (std::invalid_argument) {

        }
    }
    if (isnan(result)) {
        exit(-1);
    }
    return result;
}

void spin(double delay) {
    double timeout = 0.0;
    struct geopm_time_s start = {{0, 0}};
    struct geopm_time_s curr = {{0, 0}};
    geopm_time(&start);
    while (timeout < delay) {
        geopm_time(&curr);
        timeout = geopm_time_diff(&start, &curr);
    }
}

void *dgemm_thread(void *args) {
    const struct geopm_time_s true_zero = {{0, 0}};
    struct geopm_time_s time_zero = {{0, 0}}, start_time = {{0, 0}}, end_time = {{0, 0}};
    double *aa = NULL;
    double *bb = NULL;
    double *cc = NULL;

    /* Allocate some memory */
    const size_t num_elements = MATRIX_SIZE * (MATRIX_SIZE + PAD_SIZE);
    pthread_t thread;

    int err = posix_memalign((void **)&aa, 64, num_elements * sizeof(double));
    if (!err) {
        err = posix_memalign((void **)&bb, 64, num_elements * sizeof(double));
    }
    if (!err) {
        err = posix_memalign((void **)&cc, 64, num_elements * sizeof(double));
    }
    if (!err) {
        /* setup matrix values */
        memset(cc, 0, num_elements * sizeof(double));
        for (int i = 0; i < num_elements; ++i) {
            aa[i] = 1.0;
            bb[i] = 2.0;
        }
    }

    /* Run dgemm in a loop */
    int M = MATRIX_SIZE;
    int N = MATRIX_SIZE;
    int K = MATRIX_SIZE;
    int P = PAD_SIZE;
    int LDA = MATRIX_SIZE + PAD_SIZE;
    int LDB = MATRIX_SIZE + PAD_SIZE;
    int LDC = MATRIX_SIZE + PAD_SIZE;
    double alpha = 2.0;
    double beta = 3.0;
    char transa = 'n';
    char transb = 'n';
    geopm_time(&time_zero);
    std::cout << geopm_time_diff(&true_zero, &time_zero) << std::endl << std::endl << "start\t\t\tend" << std::endl;
    for (int i = 0; i < NUM_DGEMM_REP; ++i) {
        geopm_time(&start_time);
        dgemm_(&transa, &transb, &M, &N, &K, &alpha,
                aa, &LDA, bb, &LDB, &beta, cc, &LDC);
        geopm_time(&end_time);
        std::cout << std::setw(15) << geopm_time_diff(&time_zero, &start_time) << "\t" << std::setw(15) << geopm_time_diff(&time_zero, &end_time) << std::endl;
    }

    if (aa) free(aa);
    if (bb) free(bb);
    if (cc) free(cc);

    return NULL;
}

void *sampling_thread(void *args)
{
    const long cpu_count = num_cpu();
    const uint64_t sticker_freq = cpu_freq_sticker() / 1e8; // TODO:
    const uint64_t min_freq = cpu_freq_min() / 1e9; // TODO: why 1e9 not 8?  this API returns 1 less 0 than sticker API
    struct msr_batch_array_s msr_array[BATCH_OPS];

    for (int x = 0; x < BATCH_OPS; ++x) {
        msr_array[x].numops = cpu_count;
        msr_array[x].ops = (struct msr_batch_op_s *) malloc(sizeof(struct msr_batch_op_s) * cpu_count);
        for (int y = 0; y < cpu_count; ++y) {
            msr_array[x].ops[y].cpu = y;
            msr_array[x].ops[y].err = 0;
            switch (x) {
                case BATCH_READ:
                    msr_array[x].ops[y].msr = IA_32_PERF_STATUS_MSR;
                    msr_array[x].ops[y].isrdmsr = 1;
                    msr_array[x].ops[y].msrdata = 0;
                    msr_array[x].ops[y].wmask = 0;
                    break;
                case BATCH_WRITE_1:
                    msr_array[x].ops[y].msr = IA_32_PERF_CTL_MSR;
                    msr_array[x].ops[y].isrdmsr = 0;
                    msr_array[x].ops[y].msrdata = 0xa00;
                    msr_array[x].ops[y].wmask = IA_32_PERF_MASK;
                    break;
                case BATCH_WRITE_2:
                    msr_array[x].ops[y].msr = IA_32_PERF_CTL_MSR;
                    msr_array[x].ops[y].isrdmsr = 0;
                    msr_array[x].ops[y].msrdata = 0xb00;
                    msr_array[x].ops[y].wmask = IA_32_PERF_MASK;
                    break;
                case BATCH_WRITE_3:
                    msr_array[x].ops[y].msr = IA_32_PERF_CTL_MSR;
                    msr_array[x].ops[y].isrdmsr = 0;
                    msr_array[x].ops[y].msrdata = 0xc00;
                    msr_array[x].ops[y].wmask = IA_32_PERF_MASK;
                    break;
                case BATCH_WRITE_4:
                    msr_array[x].ops[y].msr = IA_32_PERF_CTL_MSR;
                    msr_array[x].ops[y].isrdmsr = 0;
                    msr_array[x].ops[y].msrdata = 0xd00;
                    msr_array[x].ops[y].wmask = IA_32_PERF_MASK;
                    break;
            }
        }
    }

    int m_msr_batch_desc = open(BATCH_DEVICE_PATH, O_RDWR);
    int rv;
    long cpu = 0;

#if 0
    // basic test
    std::cout << "The system has " << cpu_count << " available CPUs with frequency range [" << std::hex << min_freq << " - " << sticker_freq << std::dec << "]" << std::endl;
    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_READ]);
    printf("[%d/%d] for cpu: %d read current freq configuation of 0x%lx\n", rv, msr_array[BATCH_READ].ops[cpu].err, cpu, msr_array[BATCH_READ].ops[cpu].msrdata);
    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_WRITE_1]);
    printf("[%d/%d] for cpu: %d wrote freq configuation of 0x%lx\n", rv, msr_array[BATCH_WRITE_1].ops[cpu].err, cpu, msr_array[BATCH_WRITE_1].ops[cpu].msrdata);
    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_READ]);
    printf("[%d/%d] for cpu: %d read current freq configuation of 0x%lx\n", rv, msr_array[BATCH_READ].ops[cpu].err, cpu, msr_array[BATCH_READ].ops[cpu].msrdata);
    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_WRITE_2]);
    printf("[%d/%d] for cpu: %d wrote freq configuation of 0x%lx\n", rv, msr_array[BATCH_WRITE_2].ops[cpu].err, cpu, msr_array[BATCH_WRITE_2].ops[cpu].msrdata);
    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_READ]);
    printf("[%d/%d] for cpu: %d read current freq configuation of 0x%lx\n", rv, msr_array[BATCH_READ].ops[cpu].err, cpu, msr_array[BATCH_READ].ops[cpu].msrdata);

    return 0;
#endif

    const struct geopm_time_s true_zero = {{0, 0}};
    struct geopm_time_s time_zero = {{0, 0}}, request_time[BATCH_OPS], accept_time[BATCH_OPS], update_time[BATCH_OPS][512];// update time is a teeny bit wasteful in size but whatevs
    int write_idx = -1;

    rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_READ]);
    if (msr_array[BATCH_READ].ops[0].msrdata != msr_array[BATCH_WRITE_1].ops[0].msrdata) {
        write_idx = 0;
    }
    else if (msr_array[BATCH_READ].ops[0].msrdata != msr_array[BATCH_WRITE_2].ops[0].msrdata) {
        write_idx = 1;
    }

    geopm_time(&time_zero);
    std::cout << geopm_time_diff(&true_zero, &time_zero) << std::endl << std::endl;
    std::cout << "w_req\t\tw_ack\t\t\tfreq\tr_ack\t\t\tupdt\t\tmin\t\tmax\t\tavg\t\tstdv" << std::endl;
    for (int num_tests = 0; num_tests < NUM_TESTS; ++num_tests) {
        request_time[BATCH_READ] = {{0, 0}};
        accept_time[BATCH_READ] = {{0, 0}};
        request_time[write_idx] = {{0, 0}};
        accept_time[write_idx] = {{0, 0}};
        for (cpu = 0; cpu < cpu_count; ++cpu) {
            update_time[write_idx][cpu] = {{0, 0}};
        }
        geopm_time(&request_time[write_idx]);
        rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[write_idx]);
        geopm_time(&accept_time[write_idx]);
        int sample = 0;
        std::set<long> updated; // try unordered set?
        bool abort = false;
        bool all_updated = false;
        std::vector<double> delays(cpu_count, 0);
        while (!all_updated && !abort) {
            // take time stamp here and when spinning only spin for the amount of remaining time for this sample window
            geopm_time(&request_time[BATCH_READ]);
            rv = ioctl(m_msr_batch_desc, X86_IOC_MSR_BATCH, &msr_array[BATCH_READ]);
            geopm_time(&accept_time[BATCH_READ]);
            for (cpu = 0; cpu < cpu_count; ++cpu) {
                struct geopm_time_s ut;
                geopm_time(&ut);
                if (msr_array[write_idx].ops[cpu].msrdata == msr_array[BATCH_READ].ops[cpu].msrdata) {
                    update_time[write_idx][cpu] = ut;
                    updated.insert(cpu);
                    delays[cpu] = geopm_time_diff(&accept_time[BATCH_READ], &update_time[write_idx][cpu]);
                    if (delays[cpu] < 0) {
                        throw std::exception();
                    }
                }
            }
            spin(FREQ_SAMPLE_DELAY);
            ++sample;
            if (updated.size() == cpu_count) {
                all_updated = true;
            }
            if (sample > MAX_SAMPLES) {
                abort = true;
            }
        }

        double min = NAN, max = NAN, avg = NAN, stdv = NAN;
        if (all_updated) {
            double sum = std::accumulate(delays.begin(), delays.end(), 0.0);
            avg = sum / cpu_count;
            stdv = pow(delays[0] - avg, 2);
            min = max = delays[0];

            for (cpu = 1; cpu < cpu_count; ++cpu) {
                stdv += pow(delays[cpu] - avg, 2);
                if (delays[cpu] < min) {
                    min = delays[cpu];
                }
                if (delays[cpu] > max) {
                    max = delays[cpu];
                }
            }
        }

        std::cout.fill(' ');
        std::cout << std::setw(10) << geopm_time_diff(&time_zero, &request_time[write_idx]) << "\t" << std::setw(10) << geopm_time_diff(&time_zero, &accept_time[write_idx]) << "\t" << std::hex << std::setw(10) << msr_array[write_idx].ops[0].msrdata << std::dec << "\t" << std::setw(10) << geopm_time_diff(&time_zero, &accept_time[BATCH_READ]) << "\t" << std::setw(10) << updated.size() << "\t" << std::setw(10) << min << "\t" << std::setw(10) << max << "\t" << std::setw(10) << avg << "\t" << std::setw(10) << stdv << std::endl;
        write_idx = (++write_idx) % NUM_WRITE_BATCHES;
    }

    for (int x = 0; x < BATCH_OPS; ++x) {
        free(msr_array[x].ops);
    }

    return NULL;
}

void print_usage(void)
{
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        print_usage();
    }

    if (strcmp(SAMPLE_ARG_STR, argv[1]) == 0) {
        sampling_thread(NULL);
    }
    else if (strcmp(DGEMM_ARG_STR, argv[1]) == 0) {
        dgemm_thread(NULL);
    }
    else {
        print_usage();
    }
    return 0;
}
