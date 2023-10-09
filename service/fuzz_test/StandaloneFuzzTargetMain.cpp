/*===- StandaloneFuzzTargetMain.c - standalone main() for fuzz targets. ---===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
// This main() function can be linked to a fuzz target (i.e. a library
// that exports LLVMFuzzerTestOneInput() and possibly LLVMFuzzerInitialize())
// instead of libFuzzer. This main() function will not perform any fuzzing
// but will simply feed all input files one by one to the fuzz target.
//
// Use this file to provide reproducers for bugs when linking against libFuzzer
// or other fuzzing engine is undesirable.
//===----------------------------------------------------------------------===*/


#include <iostream>
#include "geopm/Helper.hpp"

extern "C"
{
    extern int LLVMFuzzerTestOneInput(const unsigned char *data, size_t size);
    __attribute__((weak)) extern int LLVMFuzzerInitialize(int *argc, char ***argv);
}

int main(int argc, char **argv)
{
    std::cerr << "StandaloneFuzzTargetMain: running " << argc - 1 << " inputs\n";
    if (LLVMFuzzerInitialize) {
        LLVMFuzzerInitialize(&argc, &argv);
    }
    for (int i = 1; i < argc; i++) {
        std::cerr << "Running: " << argv[i] << "\n";
        std::string buf = geopm::read_file(argv[i]);
        LLVMFuzzerTestOneInput((const unsigned char *)buf.data(), (size_t)buf.size());
        std::cerr << "Done:    " << argv[i] << ": (" << buf.size() << " bytes)\n";
    }
    return 0;
}
