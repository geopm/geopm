// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

import "testing"

func TestErrorMessage(t *testing.T) {
    expected := "<geopm> Runtime error"
    msg := ErrorMessage(ErrorRuntime)
    if msg != expected {
        t.Errorf("Error message does not match expected \"%s\" vs \"%s\"", expected, msg)
    }
}
