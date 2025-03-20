// Copyright (c) 2015 - 2025 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause
//

// This is an example of how to use the geopm golang bindings that implements
// the geopmread command line tool using the golang bindings.

package main

import (
    "flag"
    "fmt"
    "os"
    "strconv"
)

import geopm "github.com/geopm/geopm/geopmdgo/geopmdgo"

// PrintDomains prints the number of domains detected for each domain type.
func PrintDomains() error {
    board, err := geopm.NumDomain("board")
    if err != nil {
        return err
    }
    pkg, err := geopm.NumDomain("package")
    if err != nil {
        return err
    }
    core, err := geopm.NumDomain("core")
    if err != nil {
        return err
    }
    cpu, err := geopm.NumDomain("cpu")
    if err != nil {
        return err
    }
    memory, err := geopm.NumDomain("memory")
    if err != nil {
        return err
    }
    package_integrated_memory, err := geopm.NumDomain("package_integrated_memory")
    if err != nil {
        return err
    }
    nic, err := geopm.NumDomain("nic")
    if err != nil {
        return err
    }
    package_integrated_nic, err := geopm.NumDomain("package_integrated_nic")
    if err != nil {
        return err
    }
    gpu, err := geopm.NumDomain("gpu")
    if err != nil {
        return err
    }
    package_integrated_gpu, err := geopm.NumDomain("package_integrated_gpu")
    if err != nil {
        return err
    }
    gpu_chip, err := geopm.NumDomain("gpu_chip")
    if err != nil {
        return err
    }
    fmt.Printf(`board                       %d
package                     %d
core                        %d
cpu                         %d
memory                      %d
package_integrated_memory   %d
nic                         %d
package_integrated_nic      %d
gpu                         %d
package_integrated_gpu      %d
gpu_chip                    %d
`,
        board,
        pkg,
        core,
        cpu,
        memory,
        package_integrated_memory,
        nic,
        package_integrated_nic,
        gpu,
        package_integrated_gpu,
        gpu_chip)
    return nil
}

// PrintInfo prints the description of a single signal.
func PrintInfo(signalName string) error {
    description, err := geopm.SignalDescription(signalName)
    if err != nil {
        return err
    }
    fmt.Printf("%s:\n%s\n", signalName, description)
    return nil
}

// PrintInfoAll prints the descriptions of all signals.
func PrintInfoAll() error {
    signalNames, err := geopm.SignalNames()
    if err != nil {
        return err
    }
    for _, signalName := range signalNames {
        if err := PrintInfo(signalName); err != nil {
            return err
        }
    }
    return nil
}

// PrintSignals prints all available signals.
func PrintSignals() error {
    signalNames, err := geopm.SignalNames()
    if err != nil {
        return err
    }
    for _, signalName := range signalNames {
        fmt.Println(signalName)
    }
    return nil
}

// Run is the main function that processes command line arguments and calls appropriate functions.
func Run() error {
    version := flag.Bool("v", false, "print version")
    versionLong := flag.Bool("version", false, "print version")
    domain := flag.Bool("d", false, "print domains detected")
    info := flag.String("i", "", "print longer description of a signal")
    infoAll := flag.Bool("I", false, "print longer description of all signals")
    cache := flag.Bool("c", false, "Create geopm topo cache if it does not exist")
    flag.Parse()

    if *version || *versionLong {
        fmt.Printf("geopmread %s\n", "1.0.0") // Replace "1.0.0" with actual version
        return nil
    }
    if *domain {
        return PrintDomains()
    }
    if *info != "" {
        return PrintInfo(*info)
    }
    if *infoAll {
        return PrintInfoAll()
    }
    if *cache {
        return geopm.CreateCache()
    }

    args := flag.Args()
    if len(args) == 0 {
        return PrintSignals()
    }
    if len(args) == 3 {
        domainIdx, err := strconv.Atoi(args[2])
        if err != nil {
            return fmt.Errorf("invalid domain index: %s", args[2])
        }
        domainType, err := geopm.DomainType(args[1])
        if err != nil {
            return err
        }
        signal, err := geopm.ReadSignal(args[0], domainType, domainIdx)
        if err != nil {
            return err
        }
        _, formatType, _, err := geopm.SignalInfo(args[0])
        if err != nil {
            return err
        }
        formattedSignal, err := geopm.FormatSignal(signal, formatType)
        if err != nil {
            return err
        }
        fmt.Println(formattedSignal)
        return nil
    }

    return fmt.Errorf("When REQUEST is specified, all three parameters must be given: SIGNAL DOMAIN_TYPE DOMAIN_INDEX")
}

// Main function that handles errors and executes Run function.
func Main() int {
    if err := Run(); err != nil {
        if _, err2 := fmt.Fprintf(os.Stderr, "Error: %v\n", err); err2 != nil {
            return 2
        }
        return 1
    }
    return 0
}

func main() {
    os.Exit(Main())
}