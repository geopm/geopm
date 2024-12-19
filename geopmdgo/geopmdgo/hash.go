// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

/*
#cgo LDFLAGS: -lgeopmd
#include <geopm_hash.h>
#include <stdlib.h>
*/
import "C"
import (
    "unsafe"
)

// HashStr returns the GEOPM hash of a string.
func HashStr(key string) uint32 {
    keyNameCStr := C.CString(key)
    defer C.free(unsafe.Pointer(keyNameCStr))

    return uint32(C.geopm_crc32_str(keyNameCStr))
}
