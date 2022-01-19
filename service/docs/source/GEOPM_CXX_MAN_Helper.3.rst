.. role:: raw-html-m2r(raw)
   :format: html


geopm::Helper -- common helper methods
======================================






SYNOPSIS
--------

#include `<geopm/Helper.hpp> <https://github.com/geopm/geopm/blob/dev/src/Helper.hpp>`_\ 

Link with ``-lgeopm`` **(MPI)** or ``-lgeopmpolicy`` **(non-MPI)**


.. code-block:: c++

       template <class Type, class ...Args>
       unique_ptr<Type> make_unique(Args &&...args);

       string read_file(const string &path);

       double read_double_from_file(const string &path,
                                    const string &expected_units);

       void write_file(const string &path, const string &contents);

       vector<string> string_split(const string &str,
                                   const string &delim);

       string string_join(const vector<string> &string_list,
                          const string &delim);

       string hostname(void);

       vector<string> list_directory_files(const string &path);

       bool string_begins_with(const string &str, const string &key);

       bool string_ends_with(string str, string key);

       function<string(double)> string_format_type_to_function(int format_type);

       int string_format_function_to_type(function<string(double)> format_function);

       string string_format_double(double signal);

       string string_format_float(double signal);

       string string_format_integer(double signal);

       string string_format_hex(double signal);

       string string_format_raw64(double signal);

       void check_hint(uint64_t hint);

       string get_env(const string &name);

       unsigned int pid_to_uid(const int pid);

       unsigned int pid_to_gid(const int pid);

DESCRIPTION
-----------

The *Helper.hpp* file defines various utility functions.


* 
  ``make_unique()``:
  Implementation of ``std::make_unique`` (C++14) for C++11, scoped to
  the ``geopm::`` namespace.  Note that this version will only work for
  non-array types.

* 
  ``read_file()``:
  Reads the file specified by *path* and returns the contents in a string.

* 
  ``read_double_from_file()``:
  Reads the file specified by *path* and return a double read from the file.
  Provide *expected_units* to follow the double in the file.
  Provide an empty string as the argument if no units are expected.
  If a double cannot be read from the file or the units reported
  in the file do not match the expected units, an exception is thrown.

* 
  ``write_file()``:
  Writes a string *contents* to a file specified by *path*.
  This will replace the file if it exists or create it if it does not exist.

* 
  ``string_split()``:
  Splits a string *str* according to a delimiter *delim* and returns
  the string pieces in a vector.  The delimiter cannot be empty.

* 
  ``string_join()``:
  Joins a vector of strings *string_list* together with a delimter *delim*,
  and return the joined string.

* 
  ``hostname()``:
  Returns the current hostname as a string.

* 
  ``list_directory_files()``:
  List all files in the given directory specified by *path*.

* 
  ``string_begins_with()``:
  Determine whether the string *str* begins with the string *key* (a prefix).

* 
  ``string_ends_with()``:
  Determine whether the string *str* ends with the string *key* (a suffix).

* 
  ``string_format_type_to_function()``:
  Convert a format type ``enum string_format_e`` to a format function

* 
  ``string_format_function_to_type()``:
  Convert a *format_function* to a format type ``enum string_format_e``

* 
  ``string_format_double()``:
  Format a string to best represent a signal encoding a double precision floating point number.
  Input *signal* a real number that requires many significant digits to accurately represent.
  Returns a well-formatted string representation of the *signal*.

* 
  ``string_format_float()``:
  Format a string to best represent a signal encoding a single precision floating point number.
  Input *signal* a real number that requires a few significant digits to accurately represent.
  Returns a well-formatted string representation of the *signal*.

* 
  ``string_format_integer()``:
  Format a string to best represent a signal encoding a decimal integer.
  Input *signal* an integer that is best represented as a decimal number.
  Returns a well-formatted string representation of the *signal*.

* 
  ``string_format_hex()``:
  Format a string to best represent a signal encoding an unsigned hexadecimal integer.
  Input *signal* an unsigned integer that is best represented as a hexadecimal number
  and has been assigned to a double precision number.
  Returns a well-formatted string representation of the *signal*.

* 
  ``string_format_raw64()``:
  Format a string to represent the raw memory supporting a signal as an unsigned hexadecimal integer.
  Input *signal* a 64-bit unsigned integer that has been byte-wise copied into the memory of signal.
  Returns a well-formatted string representation of the *signal*.

* 
  ``check_hint()``:
  Verify a region's *hint* value is legal for use.

* 
  ``get_env()``:
  Read an environment variable with given *name*,
  and return the contents of the variable if present, otherwise an empty string.

* 
  ``pid_to_uid()``:
  Query for the user id assoiciated with the process id.
  Convert the *pid* process id into the *uid* user id.

* 
  ``pid_to_gid()``:
  Query for the group id assoiciated with the process id.
  Convert the *pid* process id into the *gid* group id.

SEE ALSO
--------

`geopm(7) <geopm.7.html>`_
