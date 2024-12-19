// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

import "testing"

func TestDomain(t *testing.T) {
    n , err := NumDomain(0)
    if n != 1 {
        t.Errorf("Number of boards should always be 1, got %d", n)
    }
    if err != nil {
        t.Errorf("NumDomain(0) errored %s", err)
    }
}