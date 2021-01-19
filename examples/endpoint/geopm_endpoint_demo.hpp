/*
 * Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
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

#ifndef GEOPMENDPOINTDEMO_HPP_INCLUDE
#define GEOPMENDPOINTDEMO_HPP_INCLUDE

#include <iostream>
#include <vector>
#include <set>

#include "geopm_time.h"


/// Helpers for printing

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T> &vec)
{
    os << "{";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        os << *it;
        if (it + 1 != vec.end()) {
            os << ", ";
        }
    }
    os << "}";
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T> &set)
{
    os << "(";
    for (auto it = set.begin(); it != set.end(); ) {
        os << *it;
        ++it;
        if (it != set.end()) {
            os << ", ";
        }
    }
    os << ")";
    return os;
}


std::ostream& operator<<(std::ostream& os, struct geopm_time_s time);

#endif
