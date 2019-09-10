#include <iostream>
#include "geopm_version.h"
#include "geopm/MSRIOGroup.hpp"

int main(int argc, char **argv)
{
    std::string help("Usage: "
    geopm::MSRIOGroup msriog;
    if (argc == 2) {
        if (strncmp("--help", argv[1], strlen("--help")) == 0) {
            std::cerr << "Usage: " << argv[1] << " [cpuid]\n"
                      << "        print the msr-safe whitelist for host CPU or cpuid if specified on command line.\n";
        }
        else if (strncmp("--version", argv[1], strlen("--version")) == 0) {
            std::cerr << geopm_version();
        }
        else {
            int cpuid = atoi(argv[1]);
            std::cout <<  msriog.msr_whitelist(cpuid);
        }
    }
    else {
        std::cout <<  msriog.msr_whitelist();
    }
}
