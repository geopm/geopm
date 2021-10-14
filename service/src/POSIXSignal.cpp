/*
 * Copyright (c) 2015 - 2021, Intel Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "POSIXSignal.hpp"
#include "geopm/Exception.hpp"
#include "geopm/Helper.hpp"
#include <cerrno>

namespace geopm
{

    std::unique_ptr<POSIXSignal> POSIXSignal::make_unique(void)
    {
        return geopm::make_unique<POSIXSignalImp>();
    }

    sigset_t POSIXSignalImp::make_sigset(const std::set<int> &signal_set) const
    {
        sigset_t result;
        int err = sigemptyset(&result);
        check_return(err, "sigemptyset()");
        for (const auto &it : signal_set) {
            int err = sigaddset(&result, it);
            check_return(err, "sigaddset()");
        }
        return result;
    }

#if 0 // This method is not defined any more.  Leaving the code here
      // as an example
    int POSIXSignalImp::signal_wait(const sigset_t &sigset) const
    {
        int result;
        int err = sigwait(&sigset, &result);
        check_return(err, "sigwait");
        return result;
    }
#endif


    void POSIXSignalImp::check_return(int err, const std::string &func_name) const
    {
        if (err == -1) {
            throw Exception("POSIXSignal(): POSIX signal function call " + func_name +
                            " returned an error", errno, __FILE__, __LINE__);
        }
    }
}
