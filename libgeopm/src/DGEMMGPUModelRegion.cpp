/*
 * Copyright (c) 2015 - 2025 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "DGEMMGPUModelRegion.hpp"

#include <iostream>
#include <cmath>
#include <sycl/sycl.hpp>

#include "geopm_prof.h"
#include "geopm_hint.h"
#include "geopm/Exception.hpp"
#include "geopm/Profile.hpp"
#include "geopm/Helper.hpp"

namespace geopm
{
    DGEMMGPUModelRegion::DGEMMGPUModelRegion(double big_o_in,
                                          int verbosity,
                                          bool do_imbalance,
                                          bool do_progress,
                                          bool do_unmarked)
        : ModelRegion(verbosity)
        , m_host_matrix_a(NULL)
        , m_host_matrix_b(NULL)
        , m_host_matrix_c(NULL)
        , m_queue(NULL)
        , m_matrix_a_buffer(NULL)
        , m_matrix_b_buffer(NULL)
        , m_matrix_c_buffer(NULL)
        , m_matrix_m_size(4096) // Matrix sizing chosen to
        , m_matrix_n_size(1024) // consume high GPU power without
        , m_matrix_k_size(2048) // running for too long.
        , m_pad_size(geopm::hardware_destructive_interference_size) // Pad to avoid false sharing
        , m_num_warmup(2) // Do 2 preliminary runs to "warm up the GPU"
    {
        m_name = "dgemm_gpu";
        m_do_imbalance = do_imbalance;
        m_do_progress = do_progress;
        m_do_unmarked = do_unmarked;
        ModelRegion::region(GEOPM_REGION_HINT_COMPUTE);
        big_o(big_o_in);
        warmup();
    }

    DGEMMGPUModelRegion::~DGEMMGPUModelRegion()
    {
        cleanup();
    }

    void DGEMMGPUModelRegion::cleanup(void)
    {
        if (m_matrix_c_buffer) {
            delete m_matrix_c_buffer;
            m_matrix_c_buffer = NULL;
        }
        if (m_matrix_b_buffer) {
            delete m_matrix_b_buffer;
            m_matrix_b_buffer = NULL;
        }
        if (m_matrix_a_buffer) {
            delete m_matrix_a_buffer;
            m_matrix_a_buffer = NULL;
        }
        if (m_queue) {
            delete m_queue;
            m_queue = NULL;
        }
        if (m_host_matrix_c) {
            free(m_host_matrix_c);
            m_host_matrix_c = NULL;
        }
        if (m_host_matrix_b) {
            free(m_host_matrix_b);
            m_host_matrix_b = NULL;
        }
        if (m_host_matrix_a) {
            free(m_host_matrix_a);
            m_host_matrix_a = NULL;
        }
    }

    void DGEMMGPUModelRegion::num_progress_updates(double big_o_in)
    {
        m_num_progress_updates = (uint64_t)(100.0 * big_o_in);
        if (m_num_progress_updates == 0) {
            m_num_progress_updates = 1;
        }
        (void)geopm_tprof_init(m_num_progress_updates);
    }

    void DGEMMGPUModelRegion::big_o(double big_o_in)
    {
        if (m_big_o && m_big_o != big_o_in) {
            cleanup();
        }
        geopm_prof_region("geopm_dgemm_gpu_model_region_startup", GEOPM_REGION_HINT_IGNORE, &m_start_rid);
        geopm_prof_enter(m_start_rid);

        num_progress_updates(big_o_in);

        if (big_o_in && m_big_o != big_o_in) {
            try {
                // Initialize SYCL queue for GPU execution using modern selector
                m_queue = new sycl::queue(sycl::gpu_selector_v);
                
                // Allocate host matrices
                // Allocate A: M x K
                size_t mem_size_a = sizeof(double) * (m_matrix_m_size + m_pad_size) * m_matrix_k_size;
                int err = posix_memalign((void **)&m_host_matrix_a, m_pad_size, mem_size_a);
                if (err) {
                    throw Exception("DGEMMGPUModelRegion::big_o(): posix_memalign() failed for matrix A",
                                   err, __FILE__, __LINE__);
                }
                for (size_t i = 0; i < mem_size_a / sizeof(double); ++i) {
                    m_host_matrix_a[i] = 2.0 * i;
                }
                
                // Allocate B: K x N
                size_t mem_size_b = sizeof(double) * (m_matrix_k_size + m_pad_size) * m_matrix_n_size;
                err = posix_memalign((void **)&m_host_matrix_b, m_pad_size, mem_size_b);
                if (err) {
                    free(m_host_matrix_a);
                    m_host_matrix_a = NULL;
                    throw Exception("DGEMMGPUModelRegion::big_o(): posix_memalign() failed for matrix B",
                                   err, __FILE__, __LINE__);
                }
                for (size_t i = 0; i < mem_size_b / sizeof(double); ++i) {
                    m_host_matrix_b[i] = 3.0 * i;
                }
                
                // Allocate C: M x N
                size_t mem_size_c = sizeof(double) * (m_matrix_m_size + m_pad_size) * m_matrix_n_size;
                err = posix_memalign((void **)&m_host_matrix_c, m_pad_size, mem_size_c);
                if (err) {
                    free(m_host_matrix_b);
                    free(m_host_matrix_a);
                    m_host_matrix_b = NULL;
                    m_host_matrix_a = NULL;
                    throw Exception("DGEMMGPUModelRegion::big_o(): posix_memalign() failed for matrix C",
                                   err, __FILE__, __LINE__);
                }
                for (size_t i = 0; i < mem_size_c / sizeof(double); ++i) {
                    m_host_matrix_c[i] = 0.0;
                }

                // Create SYCL buffers
                m_matrix_a_buffer = new sycl::buffer<double, 2>(
                    m_host_matrix_a, sycl::range<2>(m_matrix_m_size, m_matrix_k_size));
                m_matrix_b_buffer = new sycl::buffer<double, 2>(
                    m_host_matrix_b, sycl::range<2>(m_matrix_k_size, m_matrix_n_size));
                m_matrix_c_buffer = new sycl::buffer<double, 2>(
                    m_host_matrix_c, sycl::range<2>(m_matrix_m_size, m_matrix_n_size));

            } catch (sycl::exception &e) {
                cleanup();
                throw Exception("DGEMMGPUModelRegion::big_o(): SYCL error: " + std::string(e.what()),
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            } catch (std::exception &e) {
                cleanup();
                throw Exception("DGEMMGPUModelRegion::big_o(): Error: " + std::string(e.what()),
                               GEOPM_ERROR_RUNTIME, __FILE__, __LINE__);
            }
        }
        m_big_o = big_o_in;
        geopm_prof_exit(m_start_rid);
    }

    void DGEMMGPUModelRegion::warmup(void)
    {
        geopm_prof_enter(m_start_rid);
        for (int warmup_idx = 0; warmup_idx != m_num_warmup; ++warmup_idx) {
            run();
        }
        geopm_prof_exit(m_start_rid);
    }

    void DGEMMGPUModelRegion::run(void)
    {
        // DGEMM: C = alpha*A*B + beta*C
        double alpha = 2.0;
        double beta = 3.0;
        
        if (m_big_o != 0.0) {
            if (m_verbosity) {
                std::cout << "Executing GPU DGEMM (M=" << m_matrix_m_size
                          << ", N=" << m_matrix_n_size
                          << ", K=" << m_matrix_k_size << ")" << std::endl;
            }
            
            ModelRegion::region_enter();
            
            try {
                for (uint64_t i = 0; i < m_num_progress_updates; ++i) {
                    ModelRegion::loop_enter(i);

                    // Submit SYCL kernel for DGEMM computation
                    m_queue->submit([&](sycl::handler &cgh) {
                        auto a_acc = m_matrix_a_buffer->get_access<sycl::access::mode::read>(cgh);
                        auto b_acc = m_matrix_b_buffer->get_access<sycl::access::mode::read>(cgh);
                        auto c_acc = m_matrix_c_buffer->get_access<sycl::access::mode::read_write>(cgh);

                        // Fix the capture for kernel function - capture required variables explicitly
                        size_t m_size = m_matrix_m_size;
                        size_t n_size = m_matrix_n_size;
                        size_t k_size = m_matrix_k_size;
                        double alpha_val = alpha;
                        double beta_val = beta;
                        
                        cgh.parallel_for<class dgemmGPU>(
                            sycl::range<2>(m_size, n_size),
                            [=](sycl::id<2> idx) {
                                int i = idx[0];
                                int j = idx[1];
                                
                                double sum = 0.0;
                                for (size_t k = 0; k < k_size; ++k) {
                                    sum += a_acc[i][k] * b_acc[k][j];
                                }
                                c_acc[i][j] = alpha_val * sum + beta_val * c_acc[i][j];
                            }
                        );
                    });
                    
                    // Wait for the kernels to complete
                    m_queue->wait();
                    
                    ModelRegion::loop_exit();
                }
            } catch (sycl::exception &e) {
                std::cerr << "SYCL exception caught during kernel execution: " << e.what() << std::endl;
            }
            
            ModelRegion::region_exit();
        }
    }
}
