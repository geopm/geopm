// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

import "testing"

func TestErrorMessage(t *testing.T) {
    var expected uint32 = 0x8bd69e52
    actual := HashStr("Hello world")
    if actual != expected {
        t.Errorf("Error hash does not match expected \"0x%x\" vs \"0x%x\"", expected, actual)
    }
}
