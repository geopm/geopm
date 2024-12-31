// Copyright (c) 2015 - 2024 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//
package geopmdgo

/*
#cgo LDFLAGS: -lgeopmd
#include <geopm_topo.h>
#include <stdlib.h>
*/
import "C"
import (
    "fmt"
    "errors"
    "unsafe"
)

// NumDomain gets the number of domains available on the system of a specific domain type.
func NumDomain(domain interface{}) (int, error) {
    domainType, err := DomainType(domain)
    if err != nil {
        return 0, err
    }

    result := C.geopm_topo_num_domain(C.int(domainType))
    if result < 0 {
        return 0, fmt.Errorf("geopm_topo_num_domain() failed: %s", ErrorMessage(int(result)))
    }
    return int(result), nil
}

// DomainIdx gets the index of the domain that is local to a specific Linux logical CPU.
func DomainIdx(domain interface{}, cpuIdx int) (int, error) {
    domainType, err := DomainType(domain)
    if err != nil {
        return 0, err
    }

    result := C.geopm_topo_domain_idx(C.int(domainType), C.int(cpuIdx))
    if result < 0 {
        return 0, fmt.Errorf("geopm_topo_domain_idx() failed: %s", ErrorMessage(int(result)))
    }
    return int(result), nil
}

// DomainNested gets a list of all inner domains nested within a specified outer domain.
func DomainNested(innerDomain, outerDomain interface{}, outerIdx int) ([]int, error) {
    innerDomainType, err := DomainType(innerDomain)
    if err != nil {
        return nil, err
    }
    outerDomainType, err := DomainType(outerDomain)
    if err != nil {
        return nil, err
    }

    numDomainNested := C.geopm_topo_num_domain_nested(C.int(innerDomainType), C.int(outerDomainType))
    if numDomainNested < 0 {
        return nil, fmt.Errorf("geopm_topo_num_domain_nested() failed: %s", ErrorMessage(int(numDomainNested)))
    }

    domainNestedCArray := C.malloc(C.size_t(numDomainNested) * C.size_t(unsafe.Sizeof(C.int(0))))
    defer C.free(domainNestedCArray)

    C.geopm_topo_domain_nested(C.int(innerDomainType), C.int(outerDomainType), C.int(outerIdx), C.size_t(numDomainNested), (*C.int)(domainNestedCArray))

    result := make([]int, numDomainNested)
    domainNestedGoArray := (*[1 << 30]C.int)(domainNestedCArray)
    for i := 0; i < int(numDomainNested); i++ {
        result[i] = int(domainNestedGoArray[i])
    }
    return result, nil
}

// DomainName gets the domain name corresponding to the domain type specified.
func DomainName(domain interface{}) (string, error) {
    domainType, err := DomainType(domain)
    if err != nil {
        return "", err
    }

    nameMax := 1024
    resultCStr := C.malloc(C.size_t(nameMax))
    defer C.free(resultCStr)

    errCode := C.geopm_topo_domain_name(C.int(domainType), C.size_t(nameMax), (*C.char)(resultCStr))
    if errCode < 0 {
        return "", fmt.Errorf("geopm_topo_domain_name() failed: %s", ErrorMessage(int(errCode)))
    }
    return C.GoString((*C.char)(resultCStr)), nil
}

// DomainType returns the domain type that is associated with the provided domain string or integer.
func DomainType(domain interface{}) (int, error) {
    switch v := domain.(type) {
    case int:
        if v >= 0 && v < int(C.GEOPM_NUM_DOMAIN) {
            return v, nil
        }
        return 0, fmt.Errorf("domain_type is out of range: %d", domain)
    case string:
        domainCStr := C.CString(v)
        defer C.free(unsafe.Pointer(domainCStr))

        result := C.geopm_topo_domain_type(domainCStr)
        if result < 0 {
            return 0, fmt.Errorf("geopm_topo_domain_type() failed: %s", ErrorMessage(int(result)))
        }
        return int(result), nil
    default:
        return 0, errors.New("invalid domain type")
    }
}

// CreateCache creates a cache file for the platform topology if one does not exist.
func CreateCache() error {
    err := C.geopm_topo_create_cache()
    if err < 0 {
        return fmt.Errorf("geopm_topo_create_cache() failed: %s", ErrorMessage(int(err)))
    }
    return nil
}

