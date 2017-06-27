/*
 * Copyright (c) 2015, 2016, 2017, Intel Corporation
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

#include "geopm_plugin.h"
#include "Exception.hpp"
#include "Comm.hpp"
#include "MPIComm.hpp"
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
            static const CommFactory* getInstance();
            /// @brief DeciderFactory destructor, virtual.
            virtual ~CommFactory();
            IComm *get_comm(const std::string &description) const;
            IComm *get_comm(const IComm *in_comm) const;
            IComm *get_comm(const IComm *in_comm, int color, int key) const;
            IComm *get_comm(const IComm *in_comm, const std::string &tag, int split_type) const;
            IComm *get_comm(const IComm *in_comm, std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const;
        protected:
            /// @brief CommFactory default constructor.
            CommFactory();
    };

    IComm* geopm_get_comm(const std::string &description)
    {
        return CommFactory::getInstance()->get_comm(description);
    }

    IComm* geopm_get_comm(const IComm *in_comm)
    {
        return CommFactory::getInstance()->get_comm(in_comm);
    }

    IComm* geopm_get_comm(const IComm *in_comm, int color, int key)
    {
        return CommFactory::getInstance()->get_comm(in_comm, color, key);
    }

    IComm* geopm_get_comm(const IComm *in_comm, const std::string &tag, int split_type)
    {
        return CommFactory::getInstance()->get_comm(in_comm, tag, split_type);
    }

    IComm* geopm_get_comm(const IComm *in_comm, std::vector<int> dimensions, std::vector<int> periods, bool is_reorder)
    {
        return geopm::CommFactory::getInstance()->get_comm(in_comm, dimensions, periods, is_reorder);
    }

    const CommFactory* CommFactory::getInstance()
    {
        static CommFactory instance;
        return &instance;
    }

    CommFactory::CommFactory()
    {
    }

    CommFactory::~CommFactory()
    {
    }

    IComm* CommFactory::get_comm(const std::string &description) const
    {
        IComm *result = NULL;
        if (!description.compare(MPICOMM_DESCRIPTION)) {
            result = new MPIComm();
        }
        if (!result) {
            // If we get here, no acceptable comm was found
            std::ostringstream ex_str;
            ex_str << "Failure to instantiate Comm type: " << description;
            throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    IComm *CommFactory::get_comm(const IComm *in_comm) const
    {
        IComm *result = NULL;
        if (in_comm->comm_supported(MPICOMM_DESCRIPTION)) {
            result = new MPIComm(static_cast<const MPIComm *>(in_comm));
        }
        if (!result) {
            // If we get here, no acceptable comm was found
            std::ostringstream ex_str;
            ex_str << "Failure to duplicate Comm";
            throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    IComm *CommFactory::get_comm(const IComm *in_comm, int color, int key) const
    {
        IComm *result = NULL;
        if (in_comm->comm_supported(MPICOMM_DESCRIPTION)) {
            result = new MPIComm(static_cast<const MPIComm *>(in_comm), color, key);
        }
        if (!result) {
            // If we get here, no acceptable comm was found
            std::ostringstream ex_str;
            ex_str << "Failure to split Comm";
            throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    IComm *CommFactory::get_comm(const IComm *in_comm, const std::string &tag, int split_type) const
    {
        IComm *result = NULL;
        if (in_comm->comm_supported(MPICOMM_DESCRIPTION)) {
            result = new MPIComm(static_cast<const MPIComm *>(in_comm), tag, split_type);
        }
        if (!result) {
            // If we get here, no acceptable comm was found
            std::ostringstream ex_str;
            ex_str << "Failure to tag split Comm";
            throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }

    IComm *CommFactory::get_comm(const IComm *in_comm, std::vector<int> dimensions, std::vector<int> periods, bool is_reorder) const
    {
        IComm *result = NULL;
        if (in_comm->comm_supported(MPICOMM_DESCRIPTION)) {
            result = new MPIComm(static_cast<const MPIComm *>(in_comm), dimensions, periods, is_reorder);
        }
        if (!result) {
            // If we get here, no acceptable comm was found
            std::ostringstream ex_str;
            ex_str << "Failure to cart split Comm";
            throw Exception(ex_str.str(), GEOPM_ERROR_COMM_UNSUPPORTED, __FILE__, __LINE__);
        }

        return result;
    }
}
