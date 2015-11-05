/*
 * Copyright (c) 2015, Intel Corporation
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

#include <vector>

#include "Decider.hpp"

namespace geopm
{

    class DeciderFactory
    {
        public:
            /// DeciderFactory constructors
            DeciderFactory();
            DeciderFactory(std::unique_ptr<Decider> decider);
            /// DeciderFactory destructor
            virtual ~DeciderFactory();

            /// Returns an abstract Decider reference to a concrete decider.
            /// throws a std::invalid_argument if no acceptable Decider is found.
            Decider *decider(const std::string &description);
            /// Concrete Deciders register with the factory through this API.
            /// The unique_ptr assures that the object cannot be destroyed
            /// before it is copied.
            void register_decider(std::unique_ptr<Decider> decider);
        private:
            // Holds all registered concrete Decider instances
            std::vector<Decider*> deciders;
    };

}

#endif
