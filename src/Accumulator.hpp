/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ACCUMULATOR_HPP_INCLUDE
#define ACCUMULATOR_HPP_INCLUDE

#include <memory>

namespace geopm
{
    /// @brief Class to track the total increase of a signal while a
    ///        condition is true.
    ///
    /// There are many monotonically increasing signals provided by
    /// PlatformIO, for example: CPU_ENERGY, CPU_CYCLES_THREAD, and
    /// CPU_CYCLES_REFERENCE.  It is useful to track the amount that these
    /// signals increase while a condition is true.  In the common
    /// case, the condition is that the application is executing a
    /// particular region of code.  An example use for a
    /// SumAccumulator object is to track the increase in the amount
    /// of package energy consumed while the application was executing
    /// a particular region.
    ///
    /// The SumAccumulator is used to accumulate a signal that is
    /// monotonically increasing, e.g. energy, in order to track the
    /// portion of the total increase that occurred while the condition
    /// is true, e.g. while the application was executing a particular
    /// region.
    ///
    /// Each of these objects is specific to a signal, and it is also
    /// particular to a condition that is being tracked.  This
    /// condition may be: the application is executing a particular
    /// profiled region, or the hint signal has a particular value.  The
    /// user only calls the update() method when the condition is true
    /// (e.g. the application is within the tracked region).  The
    /// enter() and exit() API's are used to track values for the last
    /// occurrence of the condition being true.  It is expected
    /// (though not enforced) that one call to enter() proceeds each
    /// call to exit(), and these are used to update the values
    /// returned by interval_total().
    class SumAccumulator
    {
        public:
            /// @brief Factory constructor
            static std::unique_ptr<SumAccumulator> make_unique(void);
            /// @brief Virtual destructor
            virtual ~SumAccumulator() = default;
            /// @brief Called in control loop to update state.
            ///
            /// Update with the change in the signal being tracked for
            /// the sample.  This is called once in each control
            /// interval where the condition is true. The change in
            /// the signal is measured over the period of the last
            /// control interval.
            ///
            /// @param [in] delta_signal Change in the signal over the
            ///        control interval.
            virtual void update(double delta_signal) = 0;
            /// @brief Mark the beginning of an interval.
            ///
            /// Used to mark the beginning of an interval used for the
            /// interval_total() reporting.  The next call to exit()
            /// will close the interval and update the value returned
            /// by interval_total() to reflect the interval between
            /// the enter() and exit() calls.
            virtual void enter(void) = 0;
            /// @brief Mark the end of an interval.
            ///
            /// Used to mark the end of an interval that was
            /// previously started with a call to the enter() API.
            /// The call to exit() will update the value returned by
            /// interval_total() to reflect the interval since the
            /// enter() call.
            virtual void exit(void) = 0;
            /// @brief Total increase of tracked signal when condition
            ///        is true.
            ///
            /// Used to report on the total accumulated sum of all of
            /// the updates since the construction of the object.
            ///
            /// @return Sum of all values passed to update()
            virtual double total(void) const = 0;
            /// @brief Increase of tracked signal over last interval.
            ///
            /// Get the increase in the signal while the condition is
            /// true over the last interval.  An interval is defined
            /// by an enter() and exit() call.
            ///
            /// @return Sum of all values passed to update() during
            ///         last interval.
            virtual double interval_total(void) const = 0;
        protected:
            SumAccumulator() = default;
    };

    /// @brief Class to track the average value of a signal while a
    ///        condition is true.
    ///
    /// The AvgAccumulator is used to provide the average value of a
    /// signal while a condition is true, e.g. while the application
    /// was executing a particular region.
    ///
    /// Each of these objects is specific to a particular signal, and
    /// it is also particular to a condition that is being tracked.
    /// This condition may be: a particular region being profiled by
    /// the application, the epoch events, or the hint signal.  The
    /// user only calls the update() method when the condition is true
    /// (e.g. the application is within the tracked region).  The
    /// enter() and exit() API's are used to track values for the last
    /// occurrence of the condition being true.  It is expected (though
    /// not enforced) that one call to enter() proceeds each call to
    /// exit(), and these are used to update the values returned by
    /// interval_average().
    class AvgAccumulator
    {
        public:
            /// @brief Factory constructor
            static std::unique_ptr<AvgAccumulator> make_unique(void);
            /// @brief Virtual destructor
            virtual ~AvgAccumulator() = default;
            /// @brief Called in control loop to update state.
            ///
            /// Update with the time interval and the value of the
            /// signal being tracked.  This is called once in each
            /// control interval where the condition is true. The
            /// change in the time is measured over the period of the
            /// last control interval.
            ///
            /// @param [in] delta_time Change in the time over the
            ///        control interval.
            ///
            /// @param [in] signal Value of the signal being tracked.
            virtual void update(double delta_time, double signal) = 0;
            /// @brief Mark the beginning of an interval.
            ///
            /// Used to mark the beginning of an interval used for the
            /// interval_average() reporting.  The next call to exit()
            /// will close the interval and update the value returned
            /// by interval_average() to reflect the interval between
            /// the enter() and exit() calls.
            virtual void enter(void) = 0;
            /// @brief Mark the end of an interval.
            ///
            /// Used to mark the end of an interval that was
            /// previously started with a call to the enter() API.
            /// The call to exit() will update the value returned by
            /// interval_average() to reflect the interval since the
            /// enter() call.
            virtual void exit(void) = 0;
            /// @brief Average of the signal tracked while the
            ///         condition is true.
            ///
            /// Get the average value of the signal being tracked when
            /// the condition was true.  This average is weighted by
            /// the duration of the control loop when each update()
            /// call was made.
            ///
            /// @return Time weighted average of the signal being
            ///         tracked while the condition is true.
            virtual double average(void) const = 0;
            /// @brief Average of the signal tracked while the
            ///         condition is true.
            ///
            /// Get the average value of the signal being tracked when
            /// the condition was true over the last interval.  This
            /// average is weighted by the duration of the control
            /// loop when each update() call was made, and limited to
            /// the updates() made during the last enter()/exit()
            /// interval.
            ///
            /// @return Time weighted average of the signal being
            ///         tracked while the condition is true over the
            ///         last interval.
            virtual double interval_average(void) const = 0;
        protected:
            AvgAccumulator() = default;
    };

    class SumAccumulatorImp : public SumAccumulator
    {
        public:
            SumAccumulatorImp();
            virtual ~SumAccumulatorImp() = default;
            void update(double delta_signal) override;
            void enter(void) override;
            void exit(void) override;
            double total(void) const override;
            double interval_total(void) const override;
        private:
            double m_total;
            double m_current;
            double m_last;
    };

    class AvgAccumulatorImp : public AvgAccumulator
    {
        public:
            AvgAccumulatorImp();
            virtual ~AvgAccumulatorImp() = default;
            void update(double delta_time, double signal) override;
            void enter(void) override;
            void exit(void) override;
            double average(void) const override;
            double interval_average(void) const override;
        private:
            double m_total;
            double m_weight;
            double m_curr_total;
            double m_curr_weight;
            double m_last;
    };
}

#endif
