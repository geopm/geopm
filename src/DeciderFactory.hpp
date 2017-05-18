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

#ifndef DECIDERFACTORY_HPP_INCLUDE
#define DECIDERFACTORY_HPP_INCLUDE

#include <list>
#include <memory>

#include "Decider.hpp"

namespace geopm
{
    /// @brief Factory object managing decider objects.
    ///
    /// The DeciderFactory manages all instances of Decider objects. During
    /// construction the factory creates instances of all built in Decider classes
    /// as well as any Decider plugins present on the system. All Deciders then register
    /// themselves with the factory. The factory returns an appropriate Decider object
    /// when queried with a description string. The factory deletes all Decider objects
    /// on destruction.
    class IDeciderFactory
    {
        public:
            IDeciderFactory() {}
            IDeciderFactory(const IDeciderFactory &other) {}
            virtual ~IDeciderFactory() {}
            /// @brief Returns an abstract Decider pointer to a concrete decider.
            ///
            /// The concrete Decider is specific to the description string
            /// passed in to select it.
            /// throws a std::invalid_argument if no acceptable
            /// Decider is found.
            ///
            /// @param [in] description The descrition string corresponding
            /// to the desired Decider.
            virtual IDecider *decider(const std::string &description) = 0;
            /// @brief Concrete Deciders register with the factory through this API.
            ///
            /// @param [in] decider The unique_ptr to a Decider object
            /// assures that the object cannot be destroyed before it
            /// is copied.
            virtual void register_decider(IDecider *decider, void *dl_ptr) = 0;
    };

    class DeciderFactory : public IDeciderFactory
    {
        public:
            /// @brief DeciderFactory default constructor.
            DeciderFactory();
            /// @brief DeciderFactory Testing constructor.
            ///
            /// This constructor takes in
            /// a specific Decider object  and does not load plugins.
            /// It is intended to be used for testing.
            /// @param [in] decider The unique_ptr to a Decider object
            ///             assures that the object cannot be destroyed before
            ///             it is copied.
            DeciderFactory(IDecider *decider);
            /// @brief DeciderFactory destructor, virtual.
            virtual ~DeciderFactory();
            IDecider *decider(const std::string &description);
            void register_decider(IDecider *decider, void *dl_ptr);
        private:
            // @brief Holds all registered concrete Decider instances
            std::list<IDecider*> m_decider_list;
            std::list<void *> m_dl_ptr_list;
    };

}

#endif
