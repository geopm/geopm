// Copyright (c) 2015 - 2024 Intel Corporation
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
func PrintDomains() {
    board, _ := geopm.NumDomain("board")
    pkg, _ := geopm.NumDomain("package")
    core, _ := geopm.NumDomain("core")
    cpu, _ := geopm.NumDomain("cpu")
    memory, _ := geopm.NumDomain("memory")
    package_integrated_memory, _ := geopm.NumDomain("package_integrated_memory")
    nic, _ := geopm.NumDomain("nic")
    package_integrated_nic, _ := geopm.NumDomain("package_integrated_nic")
    gpu, _ := geopm.NumDomain("gpu")
    package_integrated_gpu, _ := geopm.NumDomain("package_integrated_gpu")
    gpu_chip, _ := geopm.NumDomain("gpu_chip")
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
}

// PrintInfo prints the description of a single signal.
func PrintInfo(signalName string) {
    description, _ := geopm.SignalDescription(signalName)
    fmt.Printf("%s:\n    %s\n", signalName, description)
}

// PrintInfoAll prints the descriptions of all signals.
func PrintInfoAll() {
    signalNames, _ := geopm.SignalNames()
    for _, signalName := range signalNames {
        PrintInfo(signalName)
    }
}

// PrintSignals prints all available signals.
func PrintSignals() {
    signalNames, _ := geopm.SignalNames()
    for _, signalName := range signalNames {
        fmt.Println(signalName)
    }
}

// Run is the main function that processes command line arguments and calls appropriate functions.
func Run() int {
    version := flag.Bool("v", false, "print version")
    versionLong := flag.Bool("version", false, "print version")
    domain := flag.Bool("d", false, "print domains detected")
    info := flag.String("i", "", "print longer description of a signal")
    infoAll := flag.Bool("I", false, "print longer description of all signals")
    cache := flag.Bool("c", false, "Create geopm topo cache if it does not exist")
    flag.Parse()

    if *version || *versionLong {
        fmt.Printf("geopmread %s\n", "1.0.0") // Replace "1.0.0" with actual version
        return 0
    }
    if *domain {
        PrintDomains()
        return 0
    }
    if *info != "" {
        PrintInfo(*info)
        return 0
    }
    if *infoAll {
        PrintInfoAll()
        return 0
    }
    if *cache {
        geopm.CreateCache()
        return 0
    }

    args := flag.Args()
    if len(args) == 0 {
        PrintSignals()
        return 0
    }
    if len(args) == 3 {
        domainIdx, err := strconv.Atoi(args[2])
        if err != nil {
            fmt.Fprintf(os.Stderr, "invalid domain index: %s\n", args[2])
            return 1
        }
	domainType, _ := geopm.DomainType(args[1])
        signal, _ := geopm.ReadSignal(args[0], domainType, domainIdx)
        _, formatType, _, _:= geopm.SignalInfo(args[0])
        formattedSignal, _ := geopm.FormatSignal(signal, formatType)
        fmt.Println(formattedSignal)
        return 0
    }

    fmt.Fprintln(os.Stderr, "When REQUEST is specified, all three parameters must be given: SIGNAL DOMAIN_TYPE DOMAIN_INDEX")
    return 1
}

// Main function that handles errors and executes Run function.
func Main() int {
    err := Run()
    if err != 0 {
        fmt.Fprintf(os.Stderr, "Error: %v\n", err)
    }
    return err
}

func main() {
    os.Exit(Main())
}