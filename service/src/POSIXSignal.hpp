/*
 * Copyright (c) 2015 - 2024, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef POSIXSIGNAL_HPP_INCLUDE
#define POSIXSIGNAL_HPP_INCLUDE

#include <signal.h>
#include <memory>
#include <set>

namespace geopm
{
    class POSIXSignal
    {
        public:
            /// @brief Reduced information set from siginfo_t struct
            /// defined in signal.h.
            struct m_info_s {
                int signo;  // siginfo_t::si_signo;  /* Signal number */
                int value;  // siginfo_t::si_value;  /* Signal value */
                int pid;    // siginfo_t::si_pid;    /* Sending process ID */
            };

            POSIXSignal() = default;
            virtual ~POSIXSignal() = default;
            /// @brief Factory method for POSIXSignal interface
            ///
            /// @return Unique pointer to an implementation of the
            ///         POSIXSignal interface.
            static std::unique_ptr<POSIXSignal> make_unique(void);

            /// @brief Create a sigset_t from a set of signal numbers
            ///
            /// @throw geopm::Exception upon EINVAL
            ///
            /// @param signal_set [in]: Set of all signal numbers to
            ///                         add to the sigset.
            ///
            /// @return A sigset_t that is zeroed except for
            ///         specified signals
            virtual sigset_t make_sigset(const std::set<int> &signal_set) const = 0;

            /// @brief Extract the signal number, signal value integer
            ///        and sending PID from a siginfo_t struct to
            ///        simplify mock data.
            ///
            /// @param info [in]: see sigaction(2) man page
            ///
            /// @return m_info_s reduced information set from siginfo_t struct
            virtual m_info_s reduce_info(const siginfo_t &info) const = 0;

            //------------------------------------------------------------------
            // Functions below here are wrappers around signal(7)
            // functions.  They differ only in the conversion of error
            // return values into raised geopm::Exceptions based on
            // errno value.
            // ------------------------------------------------------------------

            /// @brief Wrapper for sigwaitinfo(2) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EAGAIN, EINTR, EINVAL
            ///
            /// @remark See documentation for sigwaitinfo(2) about parameters
            ///         and return value.
            virtual int sig_wait_info(const sigset_t *sigset,
                                      siginfo_t *info) const = 0;

            /// @brief Wrapper for sigtimedwait(2) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EAGAIN, EINTR, EINVAL
            ///
            /// @remark See documentation for sigtimedwait(2) about parameters
            ///         and return value.
            virtual int sig_timed_wait(const sigset_t *sigset, siginfo_t *info,
                                       const struct timespec *timeout) const = 0;

            /// @brief Wrapper for sigqueue(3) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EINVAL, ESRCH
            ///
            /// @remark See documentation for sigqueue(3) about parameters.
            virtual void sig_queue(pid_t pid, int sig,
                                   int value) const = 0;

            /// @brief Wrapper for sigaction(2) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EFAULT, EINVAL
            ///
            /// @warning The Linux API sigaction(2) does not properly
            ///          implement error checking, so this function
            ///          checks for EFAULT if `act` or `oldact` are
            ///          `nullptr`
            ///
            /// @remark See documentation for sigaction(2) about parameters.
            virtual void sig_action(int signum, const struct sigaction *act,
                                    struct sigaction *oldact) const = 0;

            /// @brief Wrapper for sigprocmask(2) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EFAULT, EINVAL
            ///
            /// @remark See documentation for sigprocmask(2) about parameters
            ///         and return value.
            virtual void sig_proc_mask(int how, const sigset_t *sigset,
                                       sigset_t *oldset) const = 0;

            /// @brief Wrapper for sigsuspend(2) that converts errors
            ///        into Exceptions.
            ///
            /// @throw geopm::Exception upon EFAULT
            ///
            /// @remark See documentation for sigsuspend(2) about
            ///         parameters and return value. Except that it
            ///         doesn't return an error by default.
            virtual void sig_suspend(const sigset_t *mask) const = 0;
    };

    class POSIXSignalImp : public POSIXSignal
    {
        public:
            POSIXSignalImp() = default;
            virtual ~POSIXSignalImp() = default;
            sigset_t make_sigset(const std::set<int> &signal_set) const override;
            m_info_s reduce_info(const siginfo_t &info) const override;
            int sig_wait_info(const sigset_t *sigset,
                              siginfo_t *info) const override;
            int sig_timed_wait(const sigset_t *sigset, siginfo_t *info,
                               const struct timespec *timeout) const override;
            void sig_queue(pid_t pid, int sig, int value) const override;
            void sig_action(int signum, const struct sigaction *act,
                            struct sigaction *oldact) const override;
            void sig_proc_mask(int how, const sigset_t *sigset,
                               sigset_t *oldset) const override;
            void sig_suspend(const sigset_t *mask) const override;
        private:
            void check_return(int err, const std::string &func_name) const;
    };
}

#endif
