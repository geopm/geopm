
geopm::Agg(3) -- data aggregation functions
===========================================






Namespaces
----------

The ``Agg`` class and the ``Agg::m_type_e`` struct are members of
the ``namespace geopm``, but the full names, ``geopm::Agg`` and
``geopm::Agg::m_type_e``, have been abbreviated in this manual.
Similarly, the ``std::`` namespace specifier has been omitted from the
interface definitions for the following standard types: ``std::vector``\ ,
``std::pair``\ , ``std::string``\ , ``std::map``\ , and ``std::function``\ , to enable
better rendering of this manual.

Synopsis
--------

#include `<geopm/Agg.hpp> <https://github.com/geopm/geopm/blob/dev/libgeopmd/include/geopm/Agg.hpp>`_

Link with ``-lgeopm`` **(MPI)** or ``-lgeopm`` **(non-MPI)**


.. code-block:: c++

       static double Agg::sum(const vector<double> &operands);

       static double Agg::average(const vector<double> &operands);

       static double Agg::median(const vector<double> &operands);

       static double Agg::logical_and(const vector<double> &operands);

       static double Agg::logical_or(const vector<double> &operands);

       static double Agg::min(const vector<double> &operands);

       static double Agg::max(const vector<double> &operands);

       static double Agg::stddev(const vector<double> &operands);

       static double Agg::region_hash(const vector<double> &operands);

       static double Agg::region_hint(const vector<double> &operands);

       static double Agg::select_first(const vector<double> &operands);

       static double Agg::expect_same(const vector<double> &operands);

       static function<double(const vector<double> &)> Agg::name_to_function(const string &name);

       static string Agg::function_to_name(function<double(const vector<double> &)> func);

       static int Agg::function_to_type(function<double(const vector<double> &)> func);

       static function<double(const vector<double> &)> Agg::type_to_function(int agg_type);

       static string Agg::type_to_name(int agg_type);

Description
-----------

This class contains helper functions for aggregating multiple
floating-point samples to a single number.  They can be used to
simplify Agent implementation.

Enum Type
---------

Each of the following enum values corresponds to each of the aggregation helper functions.

.. code-block:: c++

       enum Agg::m_type_e {
           M_SUM,
           M_AVERAGE,
           M_MEDIAN,
           M_LOGICAL_AND,
           M_LOGICAL_OR,
           M_MIN,
           M_MAX,
           M_STDDEV,
           M_REGION_HASH,
           M_REGION_HINT,
           M_SELECT_FIRST,
           M_EXPECT_SAME,
           M_NUM_TYPE
       };

Class Methods
-------------


*
  ``sum()``:
  Returns the sum of the input *operands*.

*
  ``average()``:
  Returns the average of the input *operands*.

*
  ``median()``:
  Returns the median of the input *operands*.

*
  ``logical_and()``:
  Returns the output of logical AND over all the *operands* where
  ``0.0`` is false and all other values are true.

*
  ``logical_or()``:
  Returns the output of logical OR over all the *operands* where
  ``0.0`` is false and all other values are true.

*
  ``min()``:
  Returns the minimum value from the input *operands*.

*
  ``max()``:
  Returns the maximum value from the input *operands*.

*
  ``stddev()``:
  Returns the standard deviation of the input *operands*.

*
  ``region_hash()``:
  If all *operands* are the same, returns the common value.
  Otherwise, returns ``GEOPM_REGION_HASH_UNMARKED``.  This is intended for
  situations where all ranks in a domain must be in the same region
  to exert control for that region.

*
  ``region_hint()``:
  If all *operands* are the same, returns the common value.
  Otherwise, returns ``GEOPM_REGION_HINT_UNKNOWN``.  This is intended for
  situations where all ranks in a domain must be in the same region
  to exert control for that region.

*
  ``select_first()``:
  Returns the first value in the *operands* vector and ignores other
  values.  If the vector is empty, returns ``0.0``.

*
  ``expect_same()``:
  Returns the common value if all *operands* are the same, or NAN
  otherwise.  This function should not be used to aggregate values
  that may be interpreted as NAN such as raw register values or region
  IDs.

*
  ``name_to_function()``:
  Returns the corresponding agg function for a
  given ``string`` *name*.  If the *name* does not match
  a known function, it throws an error.

*
  ``function_to_name()``:
  Returns the corresponding agg function name for a
  given ``std::function``.  If the ``std::function`` does not match
  a known function, it throws an error.

*
  ``function_to_type()``:
  Returns the corresponding agg function type for a
  given ``std::function``.  If the ``std::function`` does not match
  a known function, it throws an error.

*
  ``type_to_function()``:
  Returns the corresponding agg function for one
  of the ``Agg::m_type_e`` enum values.  If the
  *agg_type* is out of range, it throws an error.

*
  ``type_to_name()``:
  Returns the corresponding agg function name for
  one of the ``Agg:m_type_e`` enum values.  If the
  *agg_type* is out of range, it throws an error.

See Also
--------

:doc:`geopm(7) <geopm.7>`\ ,
:doc:`geopm_hash(3) <geopm_hash.3>`
