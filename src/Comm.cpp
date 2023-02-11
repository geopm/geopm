/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "config.h"

#include "Comm.hpp"

#include <inttypes.h>
#include <cpuid.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <list>
#include <mutex>
#include <algorithm>
#include <Environment.hpp>
#include <geopm_plugin.hpp>
#ifdef GEOPM_ENABLE_MPI
#include "MPIComm.hpp"
#endif

namespace geopm
{
    const std::string Comm::M_PLUGIN_PREFIX = "libgeopmcomm_";

    CommFactory::CommFactory()
    {
#ifdef GEOPM_ENABLE_MPI
        register_plugin(geopm::MPIComm::plugin_name(),
                        geopm::MPIComm::make_plugin);
#endif
        register_plugin(geopm::NullComm::plugin_name(),
                        geopm::NullComm::make_plugin);
    }


    CommFactory &comm_factory(void)
    {
        static CommFactory instance;
        static bool is_once = true;
        static std::once_flag flag;
        if (is_once) {
            is_once = false;
            std::call_once(flag, plugin_load, Comm::M_PLUGIN_PREFIX);
        }
        return instance;
    }


    std::vector<std::string> Comm::comm_names(void)
    {
        return comm_factory().plugin_names();
    }


    std::unique_ptr<Comm> Comm::make_unique(const std::string &comm_name)
    {
        return comm_factory().make_plugin(comm_name);
    }


    std::unique_ptr<Comm> Comm::make_unique(void)
    {
        return comm_factory().make_plugin(environment().comm());
    }

    NullComm::NullComm()
        : m_window_buffers(1)
    {

    }

    std::shared_ptr<Comm> NullComm::split() const
    {
        return std::make_unique<NullComm>();
    }

    std::shared_ptr<Comm> NullComm::split(int color, int key) const
    {
        return std::make_unique<NullComm>();
    }

    std::shared_ptr<Comm> NullComm::split(const std::string &tag, int split_type) const
    {
        return std::make_unique<NullComm>();
    }

    std::shared_ptr<Comm> NullComm::split(std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const
    {
        return std::make_unique<NullComm>();
    }

    std::shared_ptr<Comm> NullComm::split_cart(std::vector<int> dimensions) const
    {
        return std::make_unique<NullComm>();
    }
    std::string NullComm::plugin_name(void)
    {
        return "NullComm";
    }

    std::unique_ptr<Comm> NullComm::make_plugin()
    {
        return std::make_unique<NullComm>();
    }

    bool NullComm::comm_supported(const std::string &description) const
    {
        return description == plugin_name();
    }

    int NullComm::cart_rank(const std::vector<int> &coords) const
    {
        return 0;
    }

    int NullComm::rank(void) const
    {
        return 0;
    }

    int NullComm::num_rank(void) const
    {
        return 1;
    }

    void NullComm::dimension_create(int num_ranks, std::vector<int> &dimension) const
    {
        if (num_ranks != 1) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        dimension = {1};
    }

    void NullComm::free_mem(void *base)
    {

    }

    void NullComm::alloc_mem(size_t size, void **base)
    {
        if (size == 0) {
            *base = nullptr;
        }
        else {
            m_window_buffers.emplace_back(size);
            m_window_buffers_map[m_window_buffers.back().data()] = m_window_buffers.size() - 1;
            *base = (void *)m_window_buffers.back().data();
        }
    }

    size_t NullComm::window_create(size_t size, void *base)
    {
        if (size == 0 || base == nullptr) {
            return 0;
        }
        auto it = m_window_buffers_map.find((char *)base);
        if (it == m_window_buffers_map.end()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): Passed a base pointer that was not created with NullComm::alloc_mem",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_window_buffers.at(it->second).size() == 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): Requesting a previously freed window",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return it->second;
    }

    void NullComm::window_destroy(size_t window_id)
    {
        if (window_id == 0) {
            return;
        }
        if (window_id >= m_window_buffers.size()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): window_id is out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (m_window_buffers.at(window_id).size() == 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): destroying a previously freed window",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        m_window_buffers.at(window_id).clear();
    }

    void NullComm::window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const
    {
        if (window_id >= m_window_buffers.size()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): window_id is out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (rank != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void NullComm::window_unlock(size_t window_id, int rank) const
    {
        if (window_id == 0) {
            return;
        }
        if (window_id >= m_window_buffers.size()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): window_id must be zero",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (rank != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    void NullComm::coordinate(int rank, std::vector<int> &coord) const
    {
        if (rank != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        coord = {};
    }

    std::vector<int> NullComm::coordinate(int rank) const
    {
        if (rank != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        return {};

    }

    void NullComm::barrier(void) const
    {

    }

    void NullComm::broadcast(void *buffer, size_t size, int root) const
    {
        if (root != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

    bool NullComm::test(bool is_true) const
    {
        return is_true;
    }

    void NullComm::reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const
    {
        if (root != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy(send_buf, send_buf + count, recv_buf);
    }

    void NullComm::gather(const void *send_buf, size_t send_size, void *recv_buf,
                                size_t recv_size, int root) const
    {
        if (root != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (recv_size != send_size) {
            throw Exception("NullComm::" + std::string(__func__) + "(): For NullComm send_size must equal recv_size",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy((char *)send_buf, (char *)send_buf + send_size, (char *)recv_buf);
    }

    void NullComm::gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                           const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const
    {
        if (root != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (recv_sizes.size() != 1 || recv_sizes[0] != send_size) {
            throw Exception("NullComm::" + std::string(__func__) + "(): For NullComm send_size must equal recv_size",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        std::copy((char *)send_buf, (char *)send_buf + send_size, (char *)recv_buf);
    }

    void NullComm::window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const
    {
        if (rank != 0) {
            throw Exception("NullComm::" + std::string(__func__) + "(): NullComm is only valid with one rank",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (window_id >= m_window_buffers.size()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): window_id is out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (disp + send_size > m_window_buffers.at(window_id).size()) {
            throw Exception("NullComm::" + std::string(__func__) + "(): copy range is out of bounds",
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        char *data_ptr = m_window_buffers.at(window_id).data() + disp;
        std::copy((char *)send_buf, (char*)send_buf + send_size, data_ptr),
        throw Exception("NullComm::" + std::string(__func__) + ": NullComm does not support communication",
                        GEOPM_ERROR_INVALID, __FILE__, __LINE__);
    }

    void NullComm::tear_down(void)
    {

    }
}
