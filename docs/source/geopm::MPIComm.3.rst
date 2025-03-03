
geopm::MPIComm(3) -- implementation of Comm using MPI
=====================================================


Namespaces
----------

The ``MPIComm`` class is a member of the ``namespace geopm``\ , but the
full name, ``geopm::MPIComm``\ , has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , and ``std::set``\ , to enable better rendering of this
manual.

Note that the ``MPIComm`` class is derived from :doc:`geopm::Comm(3) <geopm::Comm.3>` class.

Synopsis
--------

#include `"MPIComm.hpp" <https://github.com/geopm/geopm/blob/dev/libgeopm/src/MPIComm.hpp>`_

Link with ``-lgeopm`` **(MPI)**


.. code-block:: c++

       static string MPIComm::plugin_name(void);

       static unique_ptr<Comm> MPIComm::make_plugin(void);

       static MPIComm& MPIComm::comm_world(void);

       virtual shared_ptr<Comm> MPIComm::split() const override;

       virtual shared_ptr<Comm> MPIComm::split(int color, int key) const override;

       virtual shared_ptr<Comm> MPIComm::split(const string &tag, int split_type) const override;

       virtual shared_ptr<Comm> MPIComm::split(vector<int> dimensions, vector<int> periods, bool is_reorder) const override;

       virtual shared_ptr<Comm> MPIComm::split_cart(vector<int> dimensions) const override;

       virtual bool MPIComm::comm_supported(const string &description) const override;

       virtual int MPIComm::cart_rank(const vector<int> &coords) const override;

       virtual int MPIComm::rank(void) const override;

       virtual int MPIComm::num_rank(void) const override;

       virtual void MPIComm::dimension_create(int num_ranks, vector<int> &dimension) const override;

       virtual void MPIComm::alloc_mem(size_t size, void **base) override;

       virtual void MPIComm::free_mem(void *base) override;

       virtual size_t MPIComm::window_create(size_t size, void *base) override;

       virtual void MPIComm::window_destroy(size_t window_id) override;

       virtual void MPIComm::coordinate(int rank, vector<int> &coord) const override;

       virtual vector<int> MPIComm::coordinate(int rank) const override;

       virtual void MPIComm::window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const override;

       virtual void MPIComm::window_unlock(size_t window_id, int rank) const override;

       virtual void MPIComm::barrier(void) const override;

       virtual void MPIComm::broadcast(void *buffer, size_t size, int root) const override;

       virtual bool MPIComm::test(bool is_true) const override;

       virtual void MPIComm::reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const override;

       virtual void MPIComm::gather(const void *send_buf, size_t send_size, void *recv_buf,
                                    size_t recv_size, int root) const override;

       virtual void MPIComm::gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                                     const vector<size_t> &recv_sizes, const vector<off_t> &rank_offset, int root) const override;

       virtual void MPIComm::window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const override;

       void MPIComm::tear_down(void) override;

Description
-----------

This class implements the Comm abstraction (:doc:`geopm::Comm(3) <geopm::Comm.3>`) using MPI
as the underlying communication mechanism.

For more details, see the
`doxygen page <https://geopm.github.io/geopm-runtime-dox/classgeopm_1_1_m_p_i_comm.html>`_.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm::Comm(3) <geopm::Comm.3>`
