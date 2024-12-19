// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

package geopmdgo

/*
#cgo LDFLAGS: -lgeopmd
#include <geopm_pio.h>
#include <stdlib.h>
*/
import "C"
import (
    "errors"
    "unsafe"
)

// SignalNames returns all available signal names in sorted order.
func SignalNames() ([]string, error) {
    var numSignal C.int
    numSignal = C.geopm_pio_num_signal_name()
    if numSignal < 0 {
        return nil, errors.New("geopm_pio_num_signal_name() failed")
    }

    nameMax := 255
    signalNameCStr := C.malloc(C.size_t(nameMax))
    defer C.free(signalNameCStr)

    signalNames := make([]string, numSignal)
    for i := 0; i < int(numSignal); i++ {
        ret := C.geopm_pio_signal_name(C.int(i), C.size_t(nameMax), (*C.char)(signalNameCStr))
        if ret < 0 {
            return nil, errors.New("geopm_pio_signal_name() failed")
        }
        signalNames[i] = C.GoString((*C.char)(signalNameCStr))
    }
    return signalNames, nil
}

// ControlNames returns all available control names in sorted order.
func ControlNames() ([]string, error) {
    var numControl C.int
    numControl = C.geopm_pio_num_control_name()
    if numControl < 0 {
        return nil, errors.New("geopm_pio_num_control_name() failed")
    }

    nameMax := 255
    controlNameCStr := C.malloc(C.size_t(nameMax))
    defer C.free(controlNameCStr)

    controlNames := make([]string, numControl)
    for i := 0; i < int(numControl); i++ {
        ret := C.geopm_pio_control_name(C.int(i), C.size_t(nameMax), (*C.char)(controlNameCStr))
        if ret < 0 {
            return nil, errors.New("geopm_pio_control_name() failed")
        }
        controlNames[i] = C.GoString((*C.char)(controlNameCStr))
    }
    return controlNames, nil
}

// SignalDomainType returns the domain type that is native for a signal.
func SignalDomainType(signalName string) (int, error) {
    signalNameCStr := C.CString(signalName)
    defer C.free(unsafe.Pointer(signalNameCStr))

    result := C.geopm_pio_signal_domain_type(signalNameCStr)
    if result < 0 {
        return 0, errors.New("geopm_pio_signal_domain_type() failed")
    }
    return int(result), nil
}

// ControlDomainType returns the domain type that is native for a control.
func ControlDomainType(controlName string) (int, error) {
    controlNameCStr := C.CString(controlName)
    defer C.free(unsafe.Pointer(controlNameCStr))

    result := C.geopm_pio_control_domain_type(controlNameCStr)
    if result < 0 {
        return 0, errors.New("geopm_pio_control_domain_type() failed")
    }
    return int(result), nil
}

// ReadSignal reads a signal value from the platform.
func ReadSignal(signalName string, domainType, domainIdx int) (float64, error) {
    resultCdbl := C.double(0)
    signalNameCStr := C.CString(signalName)
    defer C.free(unsafe.Pointer(signalNameCStr))

    domainTypeC := C.int(domainType)
    err := C.geopm_pio_read_signal(signalNameCStr, domainTypeC, C.int(domainIdx), &resultCdbl)
    if err < 0 {
        return 0, errors.New("geopm_pio_read_signal() failed")
    }
    return float64(resultCdbl), nil
}

// WriteControl writes a control value to the platform.
func WriteControl(controlName string, domainType, domainIdx int, setting float64) error {
    controlNameCStr := C.CString(controlName)
    defer C.free(unsafe.Pointer(controlNameCStr))

    domainTypeC := C.int(domainType)
    err := C.geopm_pio_write_control(controlNameCStr, domainTypeC, C.int(domainIdx), C.double(setting))
    if err < 0 {
        return errors.New("geopm_pio_write_control() failed")
    }
    return nil
}

// PushSignal pushes a signal onto the stack of batch access signals.
func PushSignal(signalName string, domainType, domainIdx int) (int, error) {
    signalNameCStr := C.CString(signalName)
    defer C.free(unsafe.Pointer(signalNameCStr))

    domainTypeC := C.int(domainType)
    result := C.geopm_pio_push_signal(signalNameCStr, domainTypeC, C.int(domainIdx))
    if result < 0 {
        return 0, errors.New("geopm_pio_push_signal() failed")
    }
    return int(result), nil
}

// PushControl pushes a control onto the stack of batch access controls.
func PushControl(controlName string, domainType, domainIdx int) (int, error) {
    controlNameCStr := C.CString(controlName)
    defer C.free(unsafe.Pointer(controlNameCStr))

    domainTypeC := C.int(domainType)
    result := C.geopm_pio_push_control(controlNameCStr, domainTypeC, C.int(domainIdx))
    if result < 0 {
        return 0, errors.New("geopm_pio_push_control() failed")
    }
    return int(result), nil
}

// Sample samples the cached value of a single signal.
func Sample(signalIdx int) (float64, error) {
    resultCdbl := C.double(0)
    err := C.geopm_pio_sample(C.int(signalIdx), &resultCdbl)
    if err < 0 {
        return 0, errors.New("geopm_pio_sample() failed")
    }
    return float64(resultCdbl), nil
}

// Adjust updates the cached value of a single control.
func Adjust(controlIdx int, setting float64) error {
    err := C.geopm_pio_adjust(C.int(controlIdx), C.double(setting))
    if err < 0 {
        return errors.New("geopm_pio_adjust() failed")
    }
    return nil
}

// ReadBatch reads all pushed signals from the platform.
func ReadBatch() error {
    err := C.geopm_pio_read_batch()
    if err < 0 {
        return errors.New("geopm_pio_read_batch() failed")
    }
    return nil
}

// WriteBatch writes all pushed controls to the platform.
func WriteBatch() error {
    err := C.geopm_pio_write_batch()
    if err < 0 {
        return errors.New("geopm_pio_write_batch() failed")
    }
    return nil
}

// SaveControl saves the state of all controls.
func SaveControl() error {
    err := C.geopm_pio_save_control()
    if err < 0 {
        return errors.New("geopm_pio_save_control() failed")
    }
    return nil
}

// RestoreControl restores the state recorded by the last call to save_control().
func RestoreControl() error {
    err := C.geopm_pio_restore_control()
    if err < 0 {
        return errors.New("geopm_pio_restore_control() failed")
    }
    return nil
}

// SignalDescription gets a description of a signal.
func SignalDescription(signalName string) (string, error) {
    nameMax := 1024
    signalNameCStr := C.CString(signalName)
    defer C.free(unsafe.Pointer(signalNameCStr))

    resultCStr := C.malloc(C.size_t(nameMax))
    defer C.free(resultCStr)

    err := C.geopm_pio_signal_description(signalNameCStr, C.size_t(nameMax), (*C.char)(resultCStr))
    if err < 0 {
        return "", errors.New("geopm_pio_signal_description() failed")
    }
    return C.GoString((*C.char)(resultCStr)), nil
}

// ControlDescription gets a description of a control.
func ControlDescription(controlName string) (string, error) {
    nameMax := 1024
    controlNameCStr := C.CString(controlName)
    defer C.free(unsafe.Pointer(controlNameCStr))

    resultCStr := C.malloc(C.size_t(nameMax))
    defer C.free(resultCStr)

    err := C.geopm_pio_control_description(controlNameCStr, C.size_t(nameMax), (*C.char)(resultCStr))
    if err < 0 {
        return "", errors.New("geopm_pio_control_description() failed")
    }
    return C.GoString((*C.char)(resultCStr)), nil
}

// SignalInfo gets information about a signal.
func SignalInfo(signalName string) (int, int, int, error) {
    signalNameCStr := C.CString(signalName)
    defer C.free(unsafe.Pointer(signalNameCStr))

    var aggregationType, formatType, behaviorType C.int
    err := C.geopm_pio_signal_info(signalNameCStr, &aggregationType, &formatType, &behaviorType)
    if err < 0 {
        return 0, 0, 0, errors.New("geopm_pio_signal_info() failed")
    }
    return int(aggregationType), int(formatType), int(behaviorType), nil
}

// FormatSignal converts a signal into a string representation.
func FormatSignal(signal float64, formatType int) (string, error) {
    nameMax := 1024
    resultCStr := C.malloc(C.size_t(nameMax))
    defer C.free(resultCStr)

    err := C.geopm_pio_format_signal(C.double(signal), C.int(formatType), C.size_t(nameMax), (*C.char)(resultCStr))
    if err < 0 {
        return "", errors.New("geopm_pio_format_signal() failed")
    }
    return C.GoString((*C.char)(resultCStr)), nil
}

// Reset resets the GEOPM platform interface.
func Reset() {
    C.geopm_pio_reset()
}

// EnableFixedCounters enables MSR fixed counter signals.
func EnableFixedCounters() {
    WriteControl("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR0", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN0_OS", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN0_USR", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN0_PMI", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR1", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN1_OS", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN1_USR", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN1_PMI", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR2", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN2_OS", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN2_USR", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN2_PMI", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_CTRL:EN_FIXED_CTR3", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN3_OS", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN3_USR", 0, 0, 1)
    WriteControl("MSR::FIXED_CTR_CTRL:EN3_PMI", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR0", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR1", 0, 0, 0)
    WriteControl("MSR::PERF_GLOBAL_OVF_CTRL:CLEAR_OVF_FIXED_CTR2", 0, 0, 0)
}
