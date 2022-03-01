.. role:: raw-html-m2r(raw)
   :format: html


geopm::SharedMemory(3) -- abstractions for shared memory
========================================================






SYNOPSIS
--------

#include `<geopm/SharedMemory.hpp> <https://github.com/geopm/geopm/blob/dev/src/SharedMemory.hpp>`_\ 

``Link with -lgeopm (MPI) or -lgeopmpolicy (non-MPI)``


* 
  ``std::unique_ptr<SharedMemory> make_unique_owner(``\ :
  ``const std::string`` _shm\ *key*\ ``,`` :raw-html-m2r:`<br>`
  ``size_t`` *size*\ ``);``

* 
  ``std::unique_ptr<SharedMemory> make_unique_user(``\ :
  ``const std::string`` _shm\ *key*\ ``,`` :raw-html-m2r:`<br>`
  ``unsigned int`` *timeout*\ ``);``

* 
  ``void *pointer(``\ :
  ``void) const;``

* 
  ``std::string key(``\ :
  ``void) const;``

* 
  ``size_t size(``\ :
  ``void) const;``

* 
  ``void unlink(``\ :
  ``void);``

* 
  ``std::unique_ptr<SharedMemoryScopedLock> get_scoped_lock(``\ :
  ``void);``

DESCRIPTION
-----------

The SharedMemory class encapsulates the creation and use of
inter-process shared memory.  In the GEOPM runtime, shared memory is
used to communicate between the user application's MPI calls and calls
to `geopm_prof_c(3) <geopm_prof_c.3.html>`_ methods, and the Controller running on the same
node.

CLASS METHODS
-------------


* 
  ``make_unique_owner``\ ():
  Creates a shared memory region with key _shm\ *key* and *size* and
  returns a pointer to a SharedMemory object managing it.

* 
  ``make_unique_user``\ ():
  Attempts to attach to a inter-process shared memory region with
  key _shm\ *key* within *timeout* and returns a pointer to a
  SharedMemory object managing it.

* 
  ``pointer``\ ():
  Returns a pointer to the shared memory region.

* 
  ``key``\ ():
  Returns the key to the shared memory region.

* 
  ``size``\ ():
  Returns the size of the shared memory region.

* 
  ``unlink``\ ():
  Unlink the shared memory region.

* 
  ``get_scoped_lock``\ ():
  Returns a temporary object that holds the mutex lock for the
  shared memory region while in scope, and releases it when it goes
  out of scope.  This method should be called before accessing the
  memory with ``pointer``\ ().

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_
