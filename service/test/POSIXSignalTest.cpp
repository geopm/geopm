/*
 * Copyright (c) 2015 - 2022, Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "config.h"

#include <memory>
#include <unistd.h>     // for getpid(), geteuid()
#include <sys/types.h>  // for pid_t, uid_t
#include <signal.h>     // for sigprocmask() from Linux API
#include <string>       // for std::string, std::to_string()
#include <cstring>      // for memset(), memcmp()
#include <cstdint>      // for uint64_t
#include <cstdlib>      // for strtoul()
#include <vector>       // for std::vector
#include <set>          // for std::set
#include <algorithm>    // for std::set_union(), std::set_difference(), std::remove()
#include <utility>      // for std::move

#include "POSIXSignal.hpp"
#include <geopm/Helper.hpp>
#include "gtest/gtest.h"
#include "geopm_test.hpp"

using geopm::POSIXSignal;
using geopm::POSIXSignalImp;

class POSIXSignalTest : public :: testing :: Test
{
    public:
        void SetUp(void);
        void TearDown(void);
    protected:
        bool has_cap_kill(void) const;

        std::shared_ptr<POSIXSignalImp> m_posix_sig;

        std::set<int> convert_sigset(const sigset_t &the_sigset);
    private:
        sigset_t m_backup_sigset;
};

/**
 * @brief save the process's sigset before running the test fixture
 *
 * @details For testing the sigprocmask(), which modifies the process's sigset.
 * This process is running multiple tests, and we do not want the tests to leave any residue.
 * Usually variable scope would be sufficient to enforce, but we are modifying the state of the process
 * because we are testing the system calls API.
 */
void POSIXSignalTest::SetUp(void)
{
    m_posix_sig = std::make_shared<POSIXSignalImp>();

    //  `if (set == nullptr)` the current value of the signal mask is saved
    sigprocmask(SIG_SETMASK, nullptr, &m_backup_sigset);
}

/**
 * @brief restore the process's sigset after finished running the test fixture
 *
 * @see POSIXSignalTest::SetUp()
 */
void POSIXSignalTest::TearDown(void)
{
    // explicitly delete managed object
    m_posix_sig.reset();

    sigprocmask(SIG_SETMASK, &m_backup_sigset, nullptr);
}

bool POSIXSignalTest::has_cap_kill(void) const
{
    uint64_t cap = 0;
    uint64_t cap_kill = 0x20;
    pid_t pid = getpid();

    std::string status_path = "/proc/" + std::to_string(pid) + "/status";

    std::vector<std::string> file_lines;
    {
        std::string file_contents = geopm::read_file(status_path);
        file_lines = geopm::string_split(file_contents, "\n");
    }  // prevent two copies of the file from hanging around in the memory
    for (const std::string& line : file_lines) {
        if (geopm::string_begins_with(line, "CapEff:")) {
            std::string temp = geopm::string_split(line, ":")[1];
            cap = strtoul(temp.c_str(), nullptr, 16);
            if (cap & cap_kill) {
                return true;
            } else {
                return false;
            }
        }
    }

    return false;
}

/**
 * @param the_sigset [in]: see sigaction(2) for information
 *
 * @return std::set<int> equivalent to the passed in sigset_t
 */
std::set<int> POSIXSignalTest::convert_sigset(const sigset_t &the_sigset)
{
    std::set<int> result;
    /// 1  ... 31 are POSIX defined signals
    /// 32 and 33 are reserved for something
    /// 34 ... 63 are user defined signals in Linux convention
    for (int signo = 1; signo < 64; ++signo)
    {
        int is_in_set = sigismember(&the_sigset, signo);
        EXPECT_NE(-1, is_in_set);
        if (is_in_set) {
            result.insert(signo);
        }
    }
    return result;
}

/**
 * @test A correct usage of make_sigset()
 */
TEST_F(POSIXSignalTest, make_sigset_correct)
{
    std::set<int> signal_set = {SIGCONT, SIGTSTP};
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    EXPECT_EQ(1, sigismember(&sigset, SIGCONT));
    EXPECT_EQ(1, sigismember(&sigset, SIGTSTP));
    EXPECT_EQ(0, sigismember(&sigset, SIGIO));
    EXPECT_EQ(0, sigismember(&sigset, SIGCHLD));
}

/**
 * @test A usage of make_sigset() with invalid parameter
 */
TEST_F(POSIXSignalTest, make_sigset_EINVAL)
{
    std::set<int> signal_set = {-1};
    std::string errmsg_expect = "Invalid argument: POSIXSignal(): POSIX signal function call sigaddset() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(m_posix_sig->make_sigset(signal_set), EINVAL, errmsg_expect);
}

/**
 * @test check that the returned sigset_t is indeed zeroed
 */
TEST_F(POSIXSignalTest, make_sigset_zeroed)
{
    std::set<int> signal_set;  // an empty set
    // convert from std::set<int> to sigset_t
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    // convert from sigset_t to std::set<int>
    std::set<int> empty_set = convert_sigset(sigset);
    // the resulting std::set<int> should be empty
    EXPECT_EQ(0U, empty_set.size());
}

/**
 * @test check that reduce_info works as expected
 */
TEST_F(POSIXSignalTest, reduce_info)
{
    siginfo_t siginfo;
    const int expect_signal = SIGCHLD;
    const int expect_value  = 4321;
    const int expect_pid    = 1234;

    siginfo.si_signo           = expect_signal;
    siginfo.si_value.sival_int = expect_value;
    siginfo.si_pid             = expect_pid;

    POSIXSignal::m_info_s info = m_posix_sig->reduce_info(siginfo);

    EXPECT_EQ(expect_signal, info.signo);
    EXPECT_EQ(expect_value,  info.value);
    EXPECT_EQ(expect_pid,    info.pid);
}

/**
 * @test A usage of sig_timed_wait() with simulated signal timeout
 */
TEST_F(POSIXSignalTest, sig_timed_wait_EAGAIN)
{
    siginfo_t info;
    timespec timeout {0,1000};
    std::set<int> signal_set = {SIGTSTP};
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    std::string errmsg_expect = "Resource temporarily unavailable: POSIXSignal(): POSIX signal function call sigtimedwait() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_timed_wait(&sigset, &info, &timeout),
        EAGAIN, errmsg_expect);
}

/**
 * @test A usage of sig_timed_wait() with invalid timeout value
 */
TEST_F(POSIXSignalTest, sig_timed_wait_EINVAL)
{
    siginfo_t info;
    timespec timeout = {-1, -1};
    std::set<int> signal_set = {SIGTSTP};
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    std::string errmsg_expect = "Invalid argument: POSIXSignal(): POSIX signal function call sigtimedwait() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_timed_wait(&sigset, &info, &timeout),
        EINVAL, errmsg_expect);
}

/**
 * @test trying to send an invalid signal
 */
TEST_F(POSIXSignalTest, sig_queue_EINVAL)
{
    std::string errmsg_expect = "Invalid argument: POSIXSignal(): POSIX signal function call sigqueue() returned an error";
    int pid = getpid();
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_queue(pid, -1, 2),
        EINVAL, errmsg_expect);
}

/**
 * @test trying to send a signal to a non existing process
 */
TEST_F(POSIXSignalTest, sig_queue_ESRCH)
{
    std::string errmsg_expect = "No such process: POSIXSignal(): POSIX signal function call sigqueue() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_queue(999999999, SIGCONT, 2),
        ESRCH, errmsg_expect);
}

/**
 * @test trying to send a signal to the init process
 *
 * @remark https://unix.stackexchange.com/a/145581
 */
TEST_F(POSIXSignalTest, sig_queue_EPERM)
{
    if (geteuid() == 0) {  // the root user
        std::cerr << "Warning: <geopm> Skipping POSIXSignalTest.sig_queue_EPERM cannot be run by user \"root\"\n";
    }
    else if (has_cap_kill()) {  // the non root user with elevated permissions
        m_posix_sig->sig_queue(1, SIGCONT, 2);
    }
    else {  // any other non root user
        std::string errmsg_expect = "Operation not permitted: POSIXSignal(): POSIX signal function call sigqueue() returned an error";
        GEOPM_EXPECT_THROW_MESSAGE(
            m_posix_sig->sig_queue(1, SIGCONT, 2),
            EPERM, errmsg_expect);
    }
}

/**
 * @test attempt is made to chenge the action for SIGKILL, which cannot be caught or ignored.
 */
TEST_F(POSIXSignalTest, sig_action_EINVAL)
{
    std::string errmsg_expect = "Invalid argument: POSIXSignal(): POSIX signal function call sigaction() returned an error";
    struct sigaction oldact;
    struct sigaction newact;
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_action(SIGKILL, &newact, &oldact),
        EINVAL, errmsg_expect);
}

/**
 * @test check if we can overwrite the current signal set
 */
TEST_F(POSIXSignalTest, sig_proc_mask_SIG_SETMASK)
{
    std::set<int> signal_set = {SIGTSTP};
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    sigset_t saved_sigset;

    // Set current signal set to argument
    m_posix_sig->sig_proc_mask(SIG_SETMASK, &sigset, nullptr);
    // Retrieve current signal set
    m_posix_sig->sig_proc_mask(SIG_SETMASK, nullptr, &saved_sigset);

    std::set<int> saved_signal_set = convert_sigset(saved_sigset);

    // Compare the two signal sets for equality to see if the signal set was changed
    EXPECT_EQ(signal_set, saved_signal_set);
}

/**
 * @test check the union of the current set and the set argument
 */
TEST_F(POSIXSignalTest, sig_proc_mask_SIG_BLOCK)
{
    std::set<int> original_signal_set   = {SIGTSTP, SIGCHLD};
    std::set<int> additional_signal_set = {SIGCHLD, SIGCONT};
    // Pre allocate enough memory to store the maximum size of the set union, as 0 values
    std::vector<int> union_signal_vector(original_signal_set.size() + additional_signal_set.size(), 0);
    std::set_union(
        original_signal_set.cbegin(), original_signal_set.cend(),
        additional_signal_set.cbegin(), additional_signal_set.cend(),
        union_signal_vector.begin()
    );
    // Erase all the 0 values from the vector
    union_signal_vector.erase(
        std::remove(union_signal_vector.begin(), union_signal_vector.end(), 0),
        union_signal_vector.end()
    );
    // Convert the std::vector into std::set
    std::set<int> union_signal_set(
        std::make_move_iterator(union_signal_vector.cbegin()),
        std::make_move_iterator(union_signal_vector.cend())
    );

    // Convert our std::set<int> into sigset_t
    sigset_t original_sigset   = m_posix_sig->make_sigset(original_signal_set);
    sigset_t additional_sigset = m_posix_sig->make_sigset(additional_signal_set);

    // Set the original sigset
    m_posix_sig->sig_proc_mask(SIG_SETMASK, &original_sigset, nullptr);
    // The set of blocked signals is the union of the original sigset and the additional sigset
    m_posix_sig->sig_proc_mask(SIG_BLOCK, &additional_sigset, nullptr);
    // Recorded the resulting value of the sigset
    sigset_t resulting_sigset;
    m_posix_sig->sig_proc_mask(SIG_SETMASK, nullptr, &resulting_sigset);
    // Convert resulting_sigset into std::set<int>
    std::set<int> resulting_set = convert_sigset(resulting_sigset);

    // Compare the two sigset_t for equality to see if the union operation succeeded
    EXPECT_EQ(union_signal_set, resulting_set);
}

/**
 * @test check unblocking signals from the current set
 */
TEST_F(POSIXSignalTest, sig_proc_mask_SIG_UNBLOCK)
{
    std::set<int> original_signal_set = {SIGTSTP, SIGCHLD};
    std::set<int> deleted_signal_set  = {SIGCHLD, SIGCONT};
    // The unblocked_signal_vector can be at most the size of the original_signal_set
    // (if deleted_signal_set didn't take out any items from the original_signal_set)
    std::vector<int> unblocked_signal_vector(original_signal_set.size(), 0);
    std::set_difference(
        original_signal_set.cbegin(), original_signal_set.cend(),
        deleted_signal_set.cbegin(), deleted_signal_set.cend(),
        unblocked_signal_vector.begin()
    );
    // Erase all the 0 values from the vector
    unblocked_signal_vector.erase(
        std::remove(unblocked_signal_vector.begin(), unblocked_signal_vector.end(), 0),
        unblocked_signal_vector.end()
    );
    // Convert the std::vector into std::set
    std::set<int> unblocked_signal_set(
        std::make_move_iterator(unblocked_signal_vector.cbegin()),
        std::make_move_iterator(unblocked_signal_vector.cend())
    );

    // Convert our std::set<int> into sigset_t
    sigset_t original_sigset  = m_posix_sig->make_sigset(original_signal_set);
    sigset_t deleted_sigset   = m_posix_sig->make_sigset(deleted_signal_set);

    // Set the original sigset
    m_posix_sig->sig_proc_mask(SIG_SETMASK, &original_sigset, nullptr);
    // The signals in deleted_sigset are removed from the current set of blocked signals.
    // It is permissible to attempt to unblock a signal which is not blocked.
    m_posix_sig->sig_proc_mask(SIG_UNBLOCK, &deleted_sigset, nullptr);
    // Recorded the resulting value of the sigset
    sigset_t resulting_sigset;
    m_posix_sig->sig_proc_mask(SIG_SETMASK, nullptr, &resulting_sigset);
    // Convert resulting_sigset into std::set<int>
    std::set<int> resulting_set = convert_sigset(resulting_sigset);

    // Compare the two sigset_t for equality to see if the difference operation succeeded
    EXPECT_EQ(unblocked_signal_set, resulting_set);
}

/**
 * @test the value specified as the first parameter was invalid
 */
TEST_F(POSIXSignalTest, sig_proc_mask_EINVAL)
{
    std::set<int> signal_set = {SIGUSR1};
    sigset_t sigset = m_posix_sig->make_sigset(signal_set);
    sigset_t old_sigset;

    std::string errmsg_expect = "Invalid argument: POSIXSignal(): POSIX signal function call sigprocmask() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_proc_mask(-1, &sigset, &old_sigset),
        EINVAL, errmsg_expect);
}

/**
 * @test the mask argument points to memory which is not a valid part of the process address space
 */
TEST_F(POSIXSignalTest, sig_suspend_EFAULT)
{
    std::string errmsg_expect = "Bad address: POSIXSignal(): POSIX signal function call sigsuspend() returned an error";
    GEOPM_EXPECT_THROW_MESSAGE(
        m_posix_sig->sig_suspend((const sigset_t*) 10),
        EFAULT, errmsg_expect);
}
