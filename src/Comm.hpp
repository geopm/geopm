/*
 * Copyright (c) 2015 - 2023, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef COMM_HPP_INCLUDE
#define COMM_HPP_INCLUDE

#include <memory>
#include <vector>
#include <string>
#include <list>

#include "geopm/PluginFactory.hpp"

namespace geopm
{
    /// @brief Abstract base class for interprocess communication in geopm
    class Comm
    {
        public:
            enum m_comm_split_type_e {
                M_COMM_SPLIT_TYPE_PPN1,
                M_COMM_SPLIT_TYPE_SHARED,
                M_NUM_COMM_SPLIT_TYPE
            };

            enum m_split_color_e {
                M_SPLIT_COLOR_UNDEFINED = -16,
            };

            /// @brief Constructor for global communicator
            Comm() = default;
            Comm(const Comm &other) = default;
            Comm &operator=(const Comm &other) = default;
            /// @brief Default destructor
            virtual ~Comm() = default;
            /// @return a list of all valid plugin names in the Comm interface
            static std::vector<std::string> comm_names(void);
            static std::unique_ptr<Comm> make_unique(const std::string &comm_name);
            static std::unique_ptr<Comm> make_unique(void);
            virtual std::shared_ptr<Comm> split() const = 0;
            virtual std::shared_ptr<Comm> split(int color, int key) const = 0;
            virtual std::shared_ptr<Comm> split(const std::string &tag, int split_type) const = 0;
            virtual std::shared_ptr<Comm> split(std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const = 0;
            virtual std::shared_ptr<Comm> split_cart(std::vector<int> dimensions) const = 0;

            virtual bool comm_supported(const std::string &description) const = 0;

            // Introspection
            /// @brief Process rank within Cartesian communicator
            ///
            /// @param [in] coords Coordinate of Cartesian communicator member
            ///        whose rank we wish to know.
            virtual int cart_rank(const std::vector<int> &coords) const = 0;
            /// @brief Process rank within communicator
            virtual int rank(void) const = 0;
            /// @brief Number of ranks in the communicator
            virtual int num_rank(void) const = 0;
            /// @brief Populate vector of optimal dimensions given the number of ranks the communicator
            ///
            /// @param [in] num_ranks Number of ranks that must fit in Cartesian grid.
            ///
            /// @param [in, out] dimension Number of ranks per dimension.  The size of this vector
            ///        dictates the number of dimensions in the grid.  Fill indices with 0 for API
            ///        to fill with suitable value.
            virtual void dimension_create(int num_ranks, std::vector<int> &dimension) const = 0;
            /// @brief Free memory that was allocated for message passing and RMA
            ///
            /// @param [in] base Address of memory to be released.
            virtual void free_mem(void *base) = 0;
            /// @brief Allocate memory for message passing and RMA
            ///
            /// @param [in] size Size of the desired memory allocation.
            ///
            /// @param [out] base Address of allocated memory.
            virtual void alloc_mem(size_t size, void **base) = 0;
            /// @brief Create window for message passing and RMA
            ///
            /// @return window handle for subsequent operations on the window.
            ///
            /// @param [in] size Size of the memory area backing the RMA window.
            ///
            /// @param [in] base Address of memory that has been allocated
            ///        for the window.
            virtual size_t window_create(size_t size, void *base) = 0;
            /// @brief Destroy window for message passing and RMA
            ///
            /// @param [in] window_id The window handle for the target window.
            virtual void window_destroy(size_t window_id) = 0;
            /// @brief Begin epoch for message passing and RMA.
            ///
            /// @param [in] window_id The window handle for the target window.
            ///
            /// @param [in] is_exclusive Lock type for the window,
            ///        true for exclusive lock, false for shared.
            ///
            /// @param [in] rank Rank of the locked window.
            ///
            /// @param [in] assert Used to optimize call.
            virtual void window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const = 0;
            /// @brief End epoch for message passing and RMA
            ///
            /// @param [in] window_id The window handle for the target window.
            ///
            /// @param [in] rank Rank of the locked window.
            virtual void window_unlock(size_t window_id, int rank) const = 0;
            /// @brief Coordinate in Cartesian grid for specified rank
            ///
            /// @param [in] rank Rank for which coordinates should be calculated
            ///
            /// @param [in, out] coord Cartesian coordinates of specified rank.
            ///        The size of this vector should equal the number of dimensions
            ///        that the Cartesian communicator was created with.
            virtual void coordinate(int rank, std::vector<int> &coord) const = 0;
            virtual std::vector<int> coordinate(int rank) const = 0;
            // Collective communication
            /// @brief Barrier for all ranks
            virtual void barrier(void) const = 0;
            /// @brief Broadcast a message to all ranks
            ///
            /// @param [in, out] buffer Starting address of buffer to be broadcasted.
            ///
            /// @param [in] size Size of the buffer.
            ///
            /// @param [in] root Rank of the broadcast root (target).
            virtual void broadcast(void *buffer, size_t size, int root) const = 0;
            /// @brief Test whether or not all ranks in the communicator present
            ///        the same input and return true/false accordingly.
            ///
            /// @param [in] is_true Boolean value to be reduced from all ranks.
            virtual bool test(bool is_true) const = 0;
            /// @brief Reduce distributed messages across all ranks using specified operation, store result on all ranks
            ///
            /// @param [in] send_buf Start address of memory buffer to be trasnmitted.
            ///
            /// @param [out] recv_buf Start address of memory buffer to receive data.
            ///
            /// @param [in] count Size of buffer in bytes to be transmitted.
            ///
            virtual void reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const = 0;
            /// @brief Gather bytes from all processes
            ///
            /// @param [in] send_buf Start address of memory buffer to be trasnmitted.
            ///
            /// @param [in] send_size Size of buffer to be sent.
            ///
            /// @param [out] recv_buf Start address of memory buffer to receive data.
            ///
            /// @param [in] recv_size The size of the buffer to be received.
            ///
            /// @param [in] root Rank of the target for the transmission.
            virtual void gather(const void *send_buf, size_t send_size, void *recv_buf,
                                size_t recv_size, int root) const = 0;
            /// @brief Gather bytes into specified location from all processes
            ///
            /// @param [in] send_buf Start address of memory buffer to be trasnmitted.
            ///
            /// @param [in] send_size Size of buffer to be sent.
            ///
            /// @param [out] recv_buf Start address of memory buffer to receive data.
            ///
            /// @param [in] recv_sizes Vector describing the buffer size per rank to be received.
            ///
            /// @param [in] rank_offset Offset per rank into target buffer for transmitted data.
            ///
            /// @param [in] root Rank of the target for the transmission.
            virtual void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                                 const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const = 0;
            /// @brief Perform message passing or RMA.
            ///
            /// @param [in] send_buf Starting address of buffer to be transmitted via window.
            ///
            /// @param [in] send_size Size in bytes of buffer to be sent.
            ///
            /// @param [in] rank Target rank of the transmission.
            ///
            /// @param [in] disp Displacement from start of window.
            ///
            /// @param [in] window_id The window handle for the target window.
            virtual void window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const = 0;
            /// @brief Clean up resources held by the comm.  This
            ///        allows static global objects to be cleaned up
            ///        before the destructor is called.
            virtual void tear_down(void) = 0;
            static const std::string M_PLUGIN_PREFIX;
    };

    class CommFactory : public PluginFactory<Comm>
    {
        public:
            CommFactory();
            virtual ~CommFactory() = default;
    };

    CommFactory &comm_factory(void);


    class NullComm : public Comm
    {
        public:
            NullComm();
            virtual ~NullComm() = default;
            std::shared_ptr<Comm> split() const override;
            std::shared_ptr<Comm> split(int color, int key) const override;
            std::shared_ptr<Comm> split(const std::string &tag, int split_type) const override;
            std::shared_ptr<Comm> split(std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const override;
            std::shared_ptr<Comm> split_cart(std::vector<int> dimensions) const override;

            bool comm_supported(const std::string &description) const override;
            int cart_rank(const std::vector<int> &coords) const override;
            int rank(void) const override;
            int num_rank(void) const override;
            void dimension_create(int num_ranks, std::vector<int> &dimension) const override;
            void free_mem(void *base) override;
            void alloc_mem(size_t size, void **base) override;
            size_t window_create(size_t size, void *base) override;
            void window_destroy(size_t window_id) override;
            void window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const override;
            void window_unlock(size_t window_id, int rank) const override;
            void coordinate(int rank, std::vector<int> &coord) const override;
            std::vector<int> coordinate(int rank) const override;
            void barrier(void) const override;
            void broadcast(void *buffer, size_t size, int root) const override;
            bool test(bool is_true) const override;
            void reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const override;
            void gather(const void *send_buf, size_t send_size, void *recv_buf,
                                size_t recv_size, int root) const override;
            void gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                                 const std::vector<size_t> &recv_sizes, const std::vector<off_t> &rank_offset, int root) const override;
            void window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const override;
            void tear_down(void) override;
            static std::string plugin_name(void);
            static std::unique_ptr<Comm> make_plugin();
        private:
            mutable std::vector<std::vector<char> > m_window_buffers;
            std::map<char *, int> m_window_buffers_map;
    };
}

#endif
