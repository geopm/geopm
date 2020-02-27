#!/usr/bin/env python2
#
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

from scipy.special import erfinv

def mean(values, stat=lambda x: x):
    return sum([stat(value) for value in values])/float(len(values))

def stddev(values, stat=lambda x: x):
    return (sum([stat(value)**2 for value in values])/float(len(values)) - mean(values, stat)**2)**0.5

# our expectation is that with N values
# probability that a value is between mu-X and mu+X is
# erf(X/(sigma * sqrt(2)))

# means probability that it is less than mu-X or greater than mu+X is
# 1 - erf(X/(sigma * sqrt(2)))

# means probability that it is above mu+X is
# (1 - erf(X/(sigma * sqrt(2))))/2

# means that we should go to 1/N

# 1/N = (1 - erf(X/(sigma * sqrt(2))))/2
# 2/N = 1 - erf(X/(sigma * sqrt(2)))
# 1 - 2/N = erf(X/(sigma * sqrt(2)))
# erfinv(1-2/N) = X/(sigma * sqrt(2))
# X = sigma * sqrt(2) * erfinv(1-2/N)

# add that to mu and there's our threshold
def outliers(value_dict, stat=lambda x: x):
    X = mean(value_dict.values(), stat) + stddev(value_dict.values(), stat) * 2**.5 * erfinv(1-2./len(value_dict))
    return [key for key in value_dict.keys() if stat(value_dict[key]) > X]

if __name__ == '__main__':
    import random
    dataset = {idx: random.gauss(0, 1) for idx in range(99)}
    dataset[99] = 5
    print(outliers(dataset))
