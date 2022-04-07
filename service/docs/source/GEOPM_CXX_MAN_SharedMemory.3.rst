.. role:: raw-html-m2r(raw)
   :format: html


geopm::SharedMemory(3) -- abstractions for shared memory
========================================================






NAMESPACES
----------

The ``SharedMemory`` is a member of the ``namespace geopm``,
but the full name ``geopm::SharedMemory`` has been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::string``\ , ``std::unique_ptr``\ , ``std::vector``\ , and ``std::function``\ , to enable
better rendering of this manual.

SYNOPSIS
--------

#include `<geopm/SharedMemory.hpp> <https://github.com/geopm/geopm/blob/dev/src/SharedMemory.hpp>`_\ 

Link with ``-lgeopmd``


.. code-block:: c++

       static unique_ptr<SharedMemory> SharedMemory::make_unique_owner(const string  &shm_key,
                                                                       size_t size);

       static unique_ptr<SharedMemory> SharedMemory::make_unique_owner_secure(const string  &shm_key,
                                                                              size_t size);

       static unique_ptr<SharedMemory> SharedMemory::make_unique_user(const string &shm_key,
                                                                      unsigned int timeout);

       void* SharedMemory::pointer(void) const;

       string SharedMemory::key(void) const;

       size_t SharedMemory::size(void) const;

       void SharedMemory::unlink(void);

       unique_ptr<SharedMemoryScopedLock> SharedMemory::get_scoped_lock(void);

       void SharedMemory::chown(const unsigned int uid,
                                const unsigned int gid);

DESCRIPTION
-----------

The ``SharedMemory`` class encapsulates the creation and use of
inter-process shared memory.  In the GEOPM runtime, shared memory is
used to communicate between the user application's MPI calls and calls
to `geopm_prof_c(3) <geopm_prof_c.3.html>`_ methods, and the Controller
running on the same node.

``SharedMemory`` is a pure virtual abstract base class.

CLASS METHODS
-------------


* 
  ``make_unique_owner()``:
  Creates a shared memory region with key *shm_key* and *size* and
  returns a pointer to a ``SharedMemory`` object managing it.

* 
  ``make_unique_owner_secure()``:
  Creates a shared memory region with key *shm_key* and *size*
  without group or world permissions and
  returns a pointer to a ``SharedMemory`` object managing it.

* 
  ``make_unique_user()``:
  Attempts to attach to a inter-process shared memory region with
  key *shm_key* within *timeout* and returns a pointer to a
  ``SharedMemory`` object managing it. If it cannot attach within the timeout,
  throws an exception.

* 
  ``pointer()``:
  Returns a pointer to the shared memory region.

* 
  ``key()``:
  Returns the key to the shared memory region.

* 
  ``size()``:
  Returns the size of the shared memory region.

* 
  ``unlink()``:
  Unlink the shared memory region.

* 
  ``get_scoped_lock()``:
  Attempt to lock the mutex for the shared memory region and
  returns a temporary object that holds the mutex lock for the
  shared memory region while in scope, and releases it when it goes
  out of scope.  This method should be called before accessing the
  memory with ``pointer()``.

* 
  ``chown()``:
  Modifies the shared memory to be owned by the specified gid
  and uid if current permissions allow for the change.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_
