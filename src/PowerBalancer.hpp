/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POWERBALANCER_HPP_INCLUDE
#define POWERBALANCER_HPP_INCLUDE

#include <memory>

namespace geopm
{
    /// @brief Stay within a power cap but redistribute power to
    ///        optimize performance. An average per compute node power
    ///        maximum is maintained, but individual nodes will be
    ///        allowed above or below this average.
    class PowerBalancer
    {
        public:
            /// @brief Construct a IPowerBalancer object.
            PowerBalancer() = default;
            /// @brief Destroy a IPowerBalancer object.
            virtual ~PowerBalancer() = default;
            /// @brief Should be called at the start of application
            ///        execution with the average power cap across
            ///        compute nodes. Should be called at the end of
            ///        the second phase of execution to note that the
            ///        power savings made across all compute nodes has
            ///        been evenly redistributed.
            /// @param power_cap The new maximum power limit equal to
            ///        the current power limit plus the amount of
            ///        power saved that is being redistributed.
            virtual void power_cap(double cap) = 0;
            /// @brief The current power cap which cannot be exceeded
            ///        without breaking contract that the average
            ///        power budget across all compute nodes is
            ///        maintained.
            /// @return The current value of the power cap.
            virtual double power_cap(void) const = 0;
            /// @brief Returns the current power limit prescribed for
            ///        this node.
            /// @return The current power limit in units of Watts.
            virtual double power_limit(void) const = 0;
            /// @brief Notify power balancer that a new limit has been
            ///        set with the governor.
            /// @param limit limit that was set.
            virtual void power_limit_adjusted(double limit) = 0;
            /// @brief Update the object with a runtime measured under
            ///        the current power limit and test if the current
            ///        runtime sample is reliable such that a call
            ///        runtime_sample() can be made.
            /// @param measured_runtime Most recent measurement of the
            ///        execution time for an epoch on the node being
            ///        managed under the current power limit.
            /// @return True if a stable measurement of expected
            ///         runtime for an epoch can be made with the
            ///         runtime_sample() method, and false otherwise.
            virtual bool is_runtime_stable(double measured_runtime) = 0;
            /// @brief Return the expected execution time of an application
            ///        epoch under the current power limit.
            virtual double runtime_sample(void) const = 0;
            /// @brief Sample the measured runtimes under the current
            ///        power cap in the first phase of execution.
            ///        This measurement will be aggregated across all
            ///        compute nodes to find the largest runtime
            ///        measured.
            virtual void calculate_runtime_sample(void) = 0;
            /// @param Set the target runtime which is the largest
            ///        epoch execution time measured by any compute
            ///        node since the application began or the last
            ///        global increase to the power budget.
            /// @param largest_runtime The largest expected runtime
            ///        for one epoch across all compute nodes under
            ///        the current power budget.
            virtual void target_runtime(double largest_runtime) = 0;
            /// @brief During the second phase of execution the power
            ///        limit is decreased until the epoch runtime on
            ///        the compute node under management has increased
            ///        to the runtime of the slowest compute node.
            ///        This method is used to update the object with a
            ///        new measurement and also test if the current
            ///        power limit meets the requirements.  If the
            ///        method returns false, then the value returned
            ///        by power_limit() may have been updated.  The
            ///        new power limit should be enforced for the next
            ///        epoch execution.
            /// @param measured_runtime Most recent measurement of the
            ///        execution time for an epoch on the node being
            ///        managed under the current power limit.
            /// @return True if the current power limit is reliably
            ///         close to the target runtime and excess power
            ///         should be sent up to the root to be
            ///         redistributed, and false if more trials are
            ///         required.
            virtual bool is_target_met(double measured_runtime) = 0;
            /// @brief Query the difference between the last power cap
            ///        setting and the current power limit.  If this
            ///        method is called and it returns zero then the
            ///        trial delta used to lower the power limit is
            ///        reduced by a factor of two.
            /// @return The difference between the last power cap and
            ///         the current power limit in units of Watts.
            virtual double power_slack(void) = 0;
            /// @brief Returns a unique_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::unique_ptr<PowerBalancer> make_unique(double ctl_latency);
            /// @brief Returns a shared_ptr to a concrete object
            ///        constructed using the underlying implementation
            static std::shared_ptr<PowerBalancer> make_shared(double ctl_latency);
    };
}

#endif
