/*
 * Copyright (c) 2015, 2016, 2017, 2018, Intel Corporation
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

#include <inttypes.h>
#include <cpuid.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <list>

#include "geopm_plugin.h"
#include "Exception.hpp"
#include "Comm.hpp"
#include "config.h"

namespace geopm
{
    /// @brief Factory object exposing IComm implementation instances.
    ///
    /// The CommFactory exposes the constructors for all implementations of the IComm
    /// pure virtual base class.  The factory, if possible, news an instance of IComm
    /// and it is the caller's responsibility to delete the object when no longer needed.
    class CommFactory
    {
        public:
            static CommFactory &getInstance();
            /// @brief CommFactory destructor, virtual.
            virtual ~CommFactory();
            virtual void register_comm_imp(const IComm *in_comm);
            virtual const IComm *get_comm(const std::string &description) const;
        protected:
            /// @brief CommFactory default constructor.
            CommFactory();
            std::list<const IComm *> m_comm_imps;
    };

    CommFactory &CommFactory::getInstance()
    {
        static CommFactory instance;
        return instance;
    }

    CommFactory::CommFactory()
    {
    }

    CommFactory::~CommFactory()
    {
    }

    void CommFactory::register_comm_imp(const IComm *in_comm)
    {
        m_comm_imps.push_back(in_comm);
    }

    const IComm* CommFactory::get_comm(const std::string &description) const
    {
        for (auto imp : m_comm_imps) {
            if (imp->comm_supported(description)) {
                return imp;
            }
        }
        // If we get here, no acceptable communication implementation was found
        std::ostringstream ex_str;
        ex_str << "Failure to instantiate Comm type: " << description;
        throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);

        return NULL;
    }
}

const geopm::IComm *geopm_get_comm(const std::string &description)
{
    return geopm::CommFactory::getInstance().get_comm(description);
}

void geopm_comm_plugin_register(geopm::IComm *comm)
{
    geopm::CommFactory::getInstance().register_comm_imp(comm);
}

