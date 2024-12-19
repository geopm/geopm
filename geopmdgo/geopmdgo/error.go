// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

/*
#cgo LDFLAGS: -lgeopmd
#include <geopm_error.h>
*/
import "C"

// Error constants
const (
    ErrorRuntime               = C.GEOPM_ERROR_RUNTIME
    ErrorLogic                 = C.GEOPM_ERROR_LOGIC
    ErrorInvalid               = C.GEOPM_ERROR_INVALID
    ErrorFileParse             = C.GEOPM_ERROR_FILE_PARSE
    ErrorLevelRange            = C.GEOPM_ERROR_LEVEL_RANGE
    ErrorNotImplemented        = C.GEOPM_ERROR_NOT_IMPLEMENTED
    ErrorPlatformUnsupported   = C.GEOPM_ERROR_PLATFORM_UNSUPPORTED
    ErrorMsrOpen               = C.GEOPM_ERROR_MSR_OPEN
    ErrorMsrRead               = C.GEOPM_ERROR_MSR_READ
    ErrorMsrWrite              = C.GEOPM_ERROR_MSR_WRITE
    ErrorAgentUnsupported      = C.GEOPM_ERROR_AGENT_UNSUPPORTED
    ErrorAffinity              = C.GEOPM_ERROR_AFFINITY
    ErrorNoAgent               = C.GEOPM_ERROR_NO_AGENT
)

// ErrorMessage returns the error message associated with the error code.
func ErrorMessage(errNumber int) string {
    pathMax := 4096
    resultCStr := C.malloc(C.size_t(pathMax))
    defer C.free(resultCStr)

    C.geopm_error_message(C.int(errNumber), (*C.char)(resultCStr), C.size_t(pathMax))
    return C.GoString((*C.char)(resultCStr))
}
