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
#include <sstream>
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
void msr_path(bool safe, int cpu_idx,
        std::string &path);
int open_msr(bool safe, int cpu_idx);
uint64_t read_msr(int msr_desc,
        uint64_t offset);
void write_msr(int msr_desc,
        uint64_t offset,
        uint64_t raw_value,
        uint64_t write_mask);

int close_non_batch(std::vector<int> &msr_descs);
int read_non_batch(const std::vector<int> &msr_descs, std::vector<uint64_t> &read_vals);
int write_non_batch(const std::vector<int> &msr_descs, uint64_t write_value);
int open_non_batch(bool safe, std::vector<int> &msr_descs);
void record_if_new(std::set<long> &updated, std::vector<double> &delays, long cpu,
        struct geopm_time_s &start_time, struct geopm_time_s &end_time);
void *sampling_thread(int argc, char **argv);
long num_cpu(void);                 /// @brief Return: number of CPUs available
double cpu_freq_sticker(void);
double cpu_freq_min(void);
void spin(double delay);
void print_usage(void);

/// macro definitions
#define SAMPLE_ARG_STR          "--sample"
#define SAMPLE_STRIDE_ARG_STR   "--stride"
#define SAMPLE_BATCH_ARG_STR    "--batch"
#define SAMPLE_SAFE_ARG_STR     "--safe"
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

#define COL_WIDTH               16
#define CONFIG_COL              std::setw(COL_WIDTH)

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
    std::cout << geopm_time_diff(&true_zero, &time_zero) << std::endl << std::endl;
    std::cout.fill(' ');
    std::cout << CONFIG_COL << "start" << CONFIG_COL << "end" << std::endl;
    for (int i = 0; i < NUM_DGEMM_REP; ++i) {
        geopm_time(&start_time);
        dgemm_(&transa, &transb, &M, &N, &K, &alpha,
                aa, &LDA, bb, &LDB, &beta, cc, &LDC);
        geopm_time(&end_time);
        std::cout << CONFIG_COL << geopm_time_diff(&time_zero, &start_time) << CONFIG_COL << geopm_time_diff(&time_zero, &end_time) << std::endl;
    }

    if (aa) free(aa);
    if (bb) free(bb);
    if (cc) free(cc);

    return NULL;
}

int open_non_batch(bool safe, std::vector<int> &msr_descs)
{
    #pragma omp parallel for
    for (int x = 0; x < msr_descs.size(); ++x) {
        msr_descs[x] = open_msr(safe, x);
    }
    return 0;
}

int write_non_batch(const std::vector<int> &msr_descs, uint64_t write_value)
{
    #pragma omp parallel for
    for (int x = 0; x < msr_descs.size(); ++x) {
        write_msr(msr_descs[x], IA_32_PERF_CTL_MSR, write_value, IA_32_PERF_MASK);
    }
    return 0;
}

int read_non_batch(const std::vector<int> &msr_descs, std::vector<uint64_t> &read_vals)
{
    #pragma omp parallel for
    for (int x = 0; x < msr_descs.size(); ++x) {
        read_vals[x] = read_msr(msr_descs[x], IA_32_PERF_STATUS_MSR);
    }
    return 0;
}

int close_non_batch(std::vector<int> &msr_descs)
{
    #pragma omp parallel for
    for (int x = 0; x < msr_descs.size(); ++x) {
        close(msr_descs[x]);
        msr_descs[x] = -1;
    }
}


void record_if_new(std::set<long> &updated, std::vector<double> &delays, long cpu,
        struct geopm_time_s &start_time, struct geopm_time_s &end_time)
{
    auto it = updated.emplace(cpu);
    if (it.second) {
        delays[cpu] = geopm_time_diff(&start_time, &end_time);
        if (delays[cpu] < 0) {
            throw std::exception();
        }
    }
}

void *sampling_thread(int argc, char **argv)
{
    const long cpu_count = num_cpu();
    const uint64_t sticker_freq = cpu_freq_sticker() / 1e8;
    const uint64_t min_freq = cpu_freq_min() / 1e9;
    const uint64_t freq_step = 0x1;
    // TODO: not all platforms support turbo
    const long total_steps = ((sticker_freq - min_freq) / freq_step) + 2; // [min ... max] + turbo
    long stride = 1;
    bool batch = false;
    bool safe = false;
    int msr_batch_desc = -1;
    int rv;

    for (int x = 0; x < argc; x++) {
        if (strcmp(SAMPLE_STRIDE_ARG_STR, argv[x]) == 0) {
            stride = atol(argv[x + 1]);
            x++;
        }
        if (strcmp(SAMPLE_BATCH_ARG_STR, argv[x]) == 0) {
            batch = true;
        }
        if (strcmp(SAMPLE_SAFE_ARG_STR, argv[x]) == 0) {
            safe = true;
        }
    }

    long num_step = 0;
    uint64_t curr_freq = min_freq;
    std::vector<uint64_t> write_vals;
    while (curr_freq <= sticker_freq + freq_step) {
        write_vals.push_back((min_freq + (stride * freq_step * num_step)) << 8);
        num_step++;
        curr_freq += freq_step * stride;
    }

    struct msr_batch_array_s write_msr_array[num_step];
    struct msr_batch_array_s read_msr_array;
    std::vector<int> msr_descs;
    std::vector<uint64_t> read_vals;

    if (batch) {
        read_msr_array.numops = cpu_count;
        read_msr_array.ops = (struct msr_batch_op_s *) malloc(sizeof(struct msr_batch_op_s) * cpu_count);
        for (int y = 0; y < cpu_count; ++y) {
            read_msr_array.ops[y].cpu = y;
            read_msr_array.ops[y].err = 0;
            read_msr_array.ops[y].msr = IA_32_PERF_STATUS_MSR;
            read_msr_array.ops[y].isrdmsr = 1;
            read_msr_array.ops[y].msrdata = 0;
            read_msr_array.ops[y].wmask = 0;
        }

        for (int x = 0; x < num_step; ++x) {
            write_msr_array[x].numops = cpu_count;
            write_msr_array[x].ops = (struct msr_batch_op_s *) malloc(sizeof(struct msr_batch_op_s) * cpu_count);
            for (int y = 0; y < cpu_count; ++y) {
                write_msr_array[x].ops[y].cpu = y;
                write_msr_array[x].ops[y].err = 0;
                write_msr_array[x].ops[y].msr = IA_32_PERF_CTL_MSR;
                write_msr_array[x].ops[y].isrdmsr = 0;
                write_msr_array[x].ops[y].msrdata = write_vals[x];
                write_msr_array[x].ops[y].wmask = IA_32_PERF_MASK;
            }
        }

        msr_batch_desc = open(BATCH_DEVICE_PATH, O_RDWR);
    }
    else {
        msr_descs.resize(cpu_count);
        std::fill(msr_descs.begin(), msr_descs.end(), -1);
        open_non_batch(safe, msr_descs);
        read_vals.resize(cpu_count);
        std::fill(read_vals.begin(), read_vals.end(), 0);
    }

#if 0
    // basic test
    long cpu = 0;
    std::cout << "min_freq: " << std::hex << min_freq << " sticker_freq: " << std::hex << sticker_freq <<
        " total_steps: " << std::dec << total_steps << " num_step: " << num_step << std::endl;
    std::cout << "The system has " << cpu_count << " available CPUs with frequency range [";
    for (uint64_t x = 0; x < num_step; ++x) {
        std::cout << "0x" << std::hex << ((min_freq + (stride * freq_step * x)) << 8);
        if (x < num_step - 1) {
            std::cout << " ";
        }
    }
    std::cout << std::dec << "]" << std::endl;

    for (int x = 0; x < num_step; ++x) {
        read_msr_array.ops[cpu].msrdata = 0x0;
        rv = ioctl(msr_batch_desc, X86_IOC_MSR_BATCH, &write_msr_array[x]);
        printf("[%d/%d] for cpu: %d %s freq configuation of 0x%lx\n", rv, write_msr_array[x].ops[cpu].err,
                cpu, write_msr_array[x].ops[cpu].isrdmsr ? "read" : "wrote", write_msr_array[x].ops[cpu].msrdata);
        int sample = 0;
        while (sample < MAX_SAMPLES && write_msr_array[x].ops[cpu].msrdata != read_msr_array.ops[cpu].msrdata){
            if (x != num_step- 1) {
                spin(FREQ_SAMPLE_DELAY);
            }
            rv = ioctl(msr_batch_desc, X86_IOC_MSR_BATCH, &read_msr_array);
            printf("[%d/%d] for cpu: %d %s current freq configuation of 0x%lx\n", rv, read_msr_array.ops[cpu].err,
                    cpu, read_msr_array.ops[cpu].isrdmsr ? "read" : "wrote", read_msr_array.ops[cpu].msrdata);
            ++sample;
        }
    }

    return 0;
#endif

    const struct geopm_time_s true_zero = {{0, 0}};
    struct geopm_time_s time_zero = {{0, 0}}, read_request_time = {{0, 0}}, read_accept_time = {{0, 0}},
                        write_request_time[num_step], write_accept_time[num_step];
    int write_idx = -1;

    if (batch) {
        rv = ioctl(msr_batch_desc, X86_IOC_MSR_BATCH, &read_msr_array);
        // OK to index 0 and 1 here as there will always be at least a min and max (and turbo actually)
        if (read_msr_array.ops[0].msrdata != write_msr_array[0].ops[0].msrdata) {
            write_idx = 0;
        }
        else if (read_msr_array.ops[0].msrdata != write_msr_array[1].ops[0].msrdata) {
            write_idx = 1;
        }
    }
    else {
        if (write_vals[0] != read_msr(msr_descs[0], IA_32_PERF_STATUS_MSR)) {
            write_idx = 0;
        }
        else {
            write_idx = 1;
        }
    }

    geopm_time(&time_zero);
    std::cout << geopm_time_diff(&true_zero, &time_zero) << std::endl << std::endl;

    std::cout << "batch (" << batch << ")"  << std::endl << "safe (" << safe << ")" << std::endl << "stride (" << stride << ")" << std::endl << std::endl;
    std::cout.fill(' ');
    std::cout << CONFIG_COL << "w_ack" << CONFIG_COL << std::hex << "frequency" << CONFIG_COL << std::dec << "updated"
        << CONFIG_COL << "min" << CONFIG_COL << "max" << CONFIG_COL << "avg" << CONFIG_COL << "stdv" << std::endl;
    for (int num_tests = 0; num_tests < NUM_TESTS; ++num_tests) {
        read_request_time = {{0, 0}};
        read_accept_time = {{0, 0}};
        write_request_time[write_idx] = {{0, 0}};
        write_accept_time[write_idx] = {{0, 0}};

        geopm_time(&write_request_time[write_idx]);
        if (batch) {
            rv = ioctl(msr_batch_desc, X86_IOC_MSR_BATCH, &write_msr_array[write_idx]);
        }
        else {
            rv = write_non_batch(msr_descs, write_vals[write_idx]);
        }
        geopm_time(&write_accept_time[write_idx]);
        struct geopm_time_s inner_done_time;
        int sample = 0;
        std::set<long> updated; // try unordered set?
        bool abort = false;
        std::vector<double> delays(cpu_count, 0.0);
        while (!abort) {
            // take time stamp here and when spinning only spin for the amount of remaining time for this sample window
            geopm_time(&read_request_time);
            if (batch) {
                rv = ioctl(msr_batch_desc, X86_IOC_MSR_BATCH, &read_msr_array);
                geopm_time(&read_accept_time);
                //  TODO SEGFAULTS, iterator needs be synchronized
                //#pragma omp parallel for
                for (long cpu = 0; cpu < cpu_count; ++cpu) {
                    if (write_vals[write_idx] == (read_msr_array.ops[cpu].msrdata & IA_32_PERF_MASK)) {
                        record_if_new(updated, delays, cpu, write_accept_time[write_idx], read_accept_time);
                    }
                }
            }
            else {
                rv = read_non_batch(msr_descs, read_vals);
                geopm_time(&read_accept_time);
                //  TODO SEGFAULTS, iterator needs be synchronized
                //#pragma omp parallel for
                for (long cpu = 0; cpu < cpu_count; ++cpu) {
                    if (write_vals[write_idx] == (read_vals[cpu] & IA_32_PERF_MASK)) {
                        record_if_new(updated, delays, cpu, write_accept_time[write_idx], read_accept_time);
                    }
                }
            }

            double min = NAN, max = NAN, avg = NAN, stdv = NAN;
            double sum = std::accumulate(delays.begin(), delays.end(), 0.0);
            avg = sum / updated.size();
            stdv = pow(delays[0] - avg, 2);
            min = max = delays[0];

            for (long cpu = 1; cpu < cpu_count; ++cpu) {
                if (delays[cpu] != 0.0) {
                    stdv += pow(delays[cpu] - avg, 2);
                }
                if (delays[cpu] < min) {
                    min = delays[cpu];
                }
                if (delays[cpu] > max) {
                    max = delays[cpu];
                }
            }

            std::cout << CONFIG_COL << geopm_time_diff(&time_zero, &write_accept_time[write_idx]) << CONFIG_COL << std::hex << write_vals[write_idx] <<
                CONFIG_COL << std::dec << updated.size() << CONFIG_COL << min << CONFIG_COL << max << CONFIG_COL << avg << CONFIG_COL << stdv << std::endl;

            geopm_time(&inner_done_time);
            double inner_work_time = geopm_time_diff(&read_request_time, &inner_done_time);
            // TODO macro for this 5
            if (inner_work_time < (FREQ_SAMPLE_DELAY * 5)) {
                spin((FREQ_SAMPLE_DELAY * 5) - inner_work_time);
            }
            ++sample;
            if (sample > MAX_SAMPLES || cpu_count == updated.size()) {
                abort = true;
            }
        }


        double outter_work_time = geopm_time_diff(&write_accept_time[write_idx], &inner_done_time);
        if (outter_work_time < (FREQ_SAMPLE_DELAY * MAX_SAMPLES)) {
            spin((FREQ_SAMPLE_DELAY * MAX_SAMPLES) - outter_work_time);
        }
        write_idx = (++write_idx) % (num_step);
    }

    if (batch) {
        free(read_msr_array.ops);
        for (int x = 0; x < num_step; ++x) {
            free(write_msr_array[x].ops);
        }
    }
    else {
        close_non_batch(msr_descs);
    }

    return NULL;
}

void msr_path(bool safe, int cpu_idx,
        std::string &path)
{
    std::ostringstream oss_msr_path;
    oss_msr_path << "/dev/cpu/" << cpu_idx;
    if (safe) {
        oss_msr_path << "/msr_safe";
    }
    else {
        oss_msr_path << "/msr";
    }
    path = oss_msr_path.str();
}

int open_msr(bool safe, int cpu_idx)
{
    int file_desc = -1;
    std::string path;
    msr_path(safe, cpu_idx, path);
    file_desc = open(path.c_str(), O_RDWR);
    if (file_desc == -1) {
        // TODO error
    }
    struct stat stat_buffer;
    int err = fstat(file_desc, &stat_buffer);
    if (err) {
        // TODO error
    }
    return file_desc;
}

uint64_t read_msr(int msr_desc,
        uint64_t offset)
{
    uint64_t result = 0;
    size_t num_read = pread(msr_desc, &result, sizeof(result), offset);
    if (num_read != sizeof(result)) {
        //std::ostringstream err_str;
        //err_str << "read_msr(): pread() failed at offset 0x" << std::hex << offset
            //<< " system error: " << strerror(errno);
        //throw Exception(err_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
    }
    return result;
}

void write_msr(int msr_desc,
        uint64_t offset,
        uint64_t raw_value,
        uint64_t write_mask)
{
    if ((raw_value & write_mask) != raw_value) {
        //std::ostringstream err_str;
        //err_str << "write_msr(): raw_value does not obey write_mask, "
            //"raw_value=0x" << std::hex << raw_value
            //<< " write_mask=0x" << write_mask;
        //throw Exception(err_str.str(), GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }
    uint64_t write_value = read_msr(msr_desc, offset);
    write_value &= ~write_mask;
    write_value |= raw_value;
    size_t num_write = pwrite(msr_desc, &write_value, sizeof(write_value), offset);
    if (num_write != sizeof(write_value)) {
        //std::ostringstream err_str;
        //err_str << "read_msr(): pwrite() failed at offset 0x" << std::hex << offset
            //<< " system error: " << strerror(errno);
        //throw Exception(err_str.str(), GEOPM_ERROR_MSR_WRITE, __FILE__, __LINE__);
    }
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
        sampling_thread(argc - 2, argv + 2);
    }
    else if (strcmp(DGEMM_ARG_STR, argv[1]) == 0) {
        dgemm_thread(NULL);
    }
    else {
        print_usage();
    }
    return 0;
}
