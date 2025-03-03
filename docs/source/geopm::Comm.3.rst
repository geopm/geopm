
geopm::Comm(3) -- communication abstractions
============================================


Namespaces
----------

The ``Comm`` class and the ``CommFactory`` class are members of
the ``namespace geopm``, but the full names, ``geopm::Comm`` and
``geopm::CommFactory``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

Synopsis
--------

#include `"Comm.hpp" <https://github.com/geopm/geopm/blob/dev/libgeopm/src/Comm.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       static vector<string> Comm::comm_names(void);

       static unique_ptr<Comm> Comm::make_unique(const string &comm_name);

       static unique_ptr<Comm> Comm::make_unique(void);

       virtual shared_ptr<Comm> Comm::split() const = 0;

       virtual shared_ptr<Comm> Comm::split(int color, int key) const = 0;

       virtual shared_ptr<Comm> Comm::split(const string &tag, int split_type) const = 0;

       virtual shared_ptr<Comm> Comm::split(vector<int> dimensions, vector<int> periods, bool is_reorder) const = 0;

       virtual shared_ptr<Comm> Comm::split_cart(vector<int> dimensions) const = 0;

       virtual bool Comm::comm_supported(const string &description) const = 0;

       virtual int Comm::cart_rank(const vector<int> &coords) const = 0;

       virtual int Comm::rank(void) const = 0;

       virtual int Comm::num_rank(void) const = 0;

       virtual void Comm::dimension_create(int num_ranks, vector<int> &dimension) const = 0;

       virtual void Comm::free_mem(void *base) = 0;

       virtual void Comm::alloc_mem(size_t size, void **base) = 0;

       virtual size_t Comm::window_create(size_t size, void *base) = 0;

       virtual void Comm::window_destroy(size_t window_id) = 0;

       virtual void Comm::window_lock(size_t window_id, bool is_exclusive, int rank, int assert) const = 0;

       virtual void Comm::window_unlock(size_t window_id, int rank) const = 0;

       virtual void Comm::coordinate(int rank, vector<int> &coord) const = 0;

       virtual vector<int> Comm::coordinate(int rank) const = 0;

       virtual void Comm::barrier(void) const = 0;

       virtual void Comm::broadcast(void *buffer, size_t size, int root) const = 0;

       virtual bool Comm::test(bool is_true) const = 0;

       virtual void Comm::reduce_max(double *send_buf, double *recv_buf, size_t count, int root) const = 0;

       virtual void Comm::gather(const void *send_buf, size_t send_size, void *recv_buf,
                                 size_t recv_size, int root) const = 0;

       virtual void Comm::gatherv(const void *send_buf, size_t send_size, void *recv_buf,
                                  const vector<size_t> &recv_sizes, const vector<off_t> &rank_offset, int root) const = 0;

       virtual void Comm::window_put(const void *send_buf, size_t send_size, int rank, off_t disp, size_t window_id) const = 0;

       virtual void Comm::tear_down(void) = 0;

Description
-----------

The ``Comm`` class is abstract base class for interprocess communication in GEOPM.

The ``CommFactory`` class is a specialization of ``PluginFactory`` class which creates instances of ``Comm`` class.

Enum Type
---------

There are two ``enum``\ s defined in the `<geopm/Comm.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopm/include/Comm.hpp>`_\ .
``Comm::m_comm_split_type_e`` and ``Comm::m_split_color_e``

.. code-block:: c++

       enum Comm::m_comm_split_type_e {
           M_COMM_SPLIT_TYPE_PPN1,
           M_COMM_SPLIT_TYPE_SHARED,
           M_NUM_COMM_SPLIT_TYPE
       };

       enum Comm::m_split_color_e {
           M_SPLIT_COLOR_UNDEFINED = -16,
       };

Factory Accessor
----------------

.. code-block:: c++

       class CommFactory : public PluginFactory<Comm>
       {
           public:
               CommFactory();
               virtual ~CommFactory() = default;
       };
       CommFactory &comm_factory(void);

* ``comm_factory()``:
  This method returns the singleton accessor for the Comm factory.
  Calling this method will create the factory if it does not already exist.


Class Methods
-------------

*
  ``comm_names()``:
  Returns a list of all valid plugin names in the Comm interface.

*
  ``make_unique()``:
  Allocate an object of requested ``Comm`` type, attached to a ``unique_ptr``,
  optionally having a *comm_name*.

*
  ``split()``:
  Allocate a new ``MPIComm`` and return it as a ``shared_ptr``,
  optionally passing in different sets of parameters to the constructor of this class.

*
  ``comm_supported()``:
  Check if the provided string *description* of the plugin is actually supported by the comm.
  The passed in *description* is the *plugin_name*, which is initialized internally by the comm.

*
  ``cart_rank()``:
  Process rank within Cartesian communicator. Pertains to Introspection.
  The const reference to a vector of integers parameter *coords* represents the
  coordinate of Cartesian communicator member whose rank we wish to know.

*
  ``rank()``:
  Get the current process rank within the communicator.

*
  ``num_rank()``:
  Get the total number of all ranks in the communicator.

*
  ``dimension_create()``:
  Populate vector of optimal dimensions given the number of ranks the communicator
  The **in** parameter *num_ranks* is the number of ranks that must fit in Cartesian grid.
  The **in, out** vector parameter *dimension* is the number of ranks per dimension.
  The size of this vector dictates the number of dimensions in the grid.
  Fill indices with 0 for API to fill with suitable value.

*
  ``free_mem()``:
  Free memory that was allocated for message passing and RMA
  The **in** parameter *base* is the address of memory to be released.
  It was created from the ``alloc_mem()`` call.

*
  ``alloc_mem()``:
  Allocate memory for message passing and RMA
  You pass in the *size* of the desired memory allocation.
  The *base* address of allocated memory is "returned" in the second parameter.

*
  ``window_create()``:
  Create window for message passing and RMA
  Return window handle for subsequent operations on the window.
  For creation we pass in the *size* of the memory area backing the RMA window,
  and the *base* address of memory that has been allocated for the window.

*
  ``window_destroy()``:
  Destroy window for message passing and RMA,
  providing it the parameter *window_id* the window handle for the target window.

*
  ``window_lock()``:
  Begin epoch for message passing and RMA.
  The parameters:
  **in** *window_id* The window handle for the target window.
  **in** *is_exclusive* Lock type for the window, true for exclusive lock, false for shared.
  **in** *rank* of the locked window.
  **in** *assert* Used to optimize call.

*
  ``window_unlock()``:
  End epoch for message passing and RMA
  The parameters:
  **in** *window_id* The window handle for the target window.
  **in** *rank* of the locked window.

*
  ``coordinate()``:
  Coordinate in Cartesian grid for specified rank
  The parameters:
  **in** *rank* for which coordinates should be calculated
  **in, out** *coord* Cartesian coordinates of specified rank.
  The size of this vector should equal the number of dimensions
  that the Cartesian communicator was created with.
  Also includes an overloaded form which takes just the *rank* as a parameter
  and returns the *coord* vector by value.

*
  ``barrier()``:
  Is a barrier for all ranks. Pertains to collective communication.

*
  ``broadcast()``:
  Broadcast a message to all ranks
  The parameters:
  **in, out** *buffer* Starting address of buffer to be broadcasted.
  **in** *size* of the buffer.
  **in** *root* Rank of the broadcast root (target).

*
  ``test()``:
  Test whether or not all ranks in the communicator present
  the same input and return *true*/*false* accordingly.
  The parameter **in** *is_true* Boolean value to be reduced from all ranks.

*
  ``reduce_max()``:
  Reduce distributed messages across all ranks using specified operation, store result on all ranks
  The parameters:
  **in** *send_buf* Start address of memory buffer to be transmitted.
  **out** *recv_buf* Start address of memory buffer to receive data.
  **in** *count* Size of buffer in bytes to be transmitted.

*
  ``gather()``:
  Gather bytes from all processes
  The parameters:
  **in** *send_buf* Start address of memory buffer to be transmitted.
  **in** *send_size* Size of buffer to be sent.
  **out** *recv_buf* Start address of memory buffer to receive data.
  **in** *recv_size* The size of the buffer to be received.
  **in** *root* Rank of the target for the transmission.

*
  ``gatherv()``:
  Gather bytes into specified location from all processes
  The parameters:
  **in** *send_buf* Start address of memory buffer to be transmitted.
  **in** *send_size* Size of buffer to be sent.
  **out** *recv_buf* Start address of memory buffer to receive data.
  **in** *recv_sizes* Vector describing the buffer size per rank to be received.
  **in** *rank_offset* Offset per rank into target buffer for transmitted data.
  **in** *root* Rank of the target for the transmission.

*
  ``window_put()``:
  Perform message passing or RMA.
  The parameters:
  **in** *send_buf* Starting address of buffer to be transmitted via window.
  **in** *send_size* Size in bytes of buffer to be sent.
  **in** *rank* Target rank of the transmission.
  **in** *disp* Displacement from start of window.
  **in** *window_id* The window handle for the target window.

*
  ``tear_down()``:
  Clean up resources held by the comm.
  This allows static global objects to be cleaned up before the destructor is called.

See Also
--------

:doc:`geopm(7) <geopm.7>`
