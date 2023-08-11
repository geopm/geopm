/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <mpi.h>
#include <unistd.h>

#include <vector>

#ifdef _OPENMP
#include <omp.h>
#else
static int omp_get_num_threads(void) {return 1;}
static int omp_get_thread_num(void) {return 0;}
#endif

#include <numeric>

int main(int argc, char **argv)
{
    size_t vector_size = 1024 * 5000;
    int err = 0;
    int comm_rank = -1;
    int comm_size = -1;
    err = MPI_Init(&argc, &argv);
    sleep(5);
    if (!err) {
        err = MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    }
    if (!err) {
        err = MPI_Comm_rank(MPI_COMM_WORLD, &comm_rank);
    }

    // we required a GPU to offload to
    if (omp_get_num_devices() == 0) {
        err = 1;
    }

    double vector_a[vector_size];
    double vector_b[vector_size];
    double vector_c[vector_size];

    // Used for validation via std c++ inner_product
    std::vector<double> validation_a;
    std::vector<double> validation_b;

    for (int i = 0; i < vector_size; i++) {
        vector_c[i] = 0;
        vector_a[i] = 2.0 * i;
        vector_b[i] = 3.0 * i;

        // should match the offload vectors
        validation_a.push_back(2.0 * i);
        validation_b.push_back(3.0 * i);
    }

    for (uint64_t i = 0; i < 5; ++i) {
        #pragma omp target teams distribute map(from:vector_c[0:vector_size]) map(to:vector_a[0:vector_size],vector_b[0:vector_size])
        for (int i = 0; i < vector_size; i++) {
            vector_c[i] = vector_a[i] * vector_b[i];
        }

        double result = 0.0;
        #pragma omp target map(tofrom:result)
        for (int i = 0; i < vector_size; i++) {
            result += vector_c[i];
            vector_c[i] = 0; // clear for the next iteration
        }

        // The result should be non-zero
        if (result == 0)  {
            err = 1;
        }

        // perform dot product using another method.
        double val_res = std::inner_product(validation_a.begin(), validation_a.end(), validation_b.begin(), 0.0);
        if (result != val_res)  {
            err = 1;
        }
    }

    MPI_Finalize();
    return err;
}
