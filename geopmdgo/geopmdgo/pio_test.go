// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

import "testing"

func TestSignalNames(t *testing.T) {
    n , err := SignalNames()
    if len(n) <= 0 {
        t.Errorf("Number of signals should always be greater than 01, got %d", len(n))
    }
    if err != nil {
        t.Errorf("SignalNames(0) errored %s", err)
    }
    found := false
    for _, sn := range n {
        if sn == "TIME" {
           found = true
        }
    }
    if ! found {
        t.Errorf("Signal names does not include \"TIME\": %s", n)
    }
}
