#include <iostream>
#include <string>
#include "geopm_version.h"
#include "MSRIOGroup.hpp"

int main(int argc, char **argv)
{
    if (argc == 2) {
        std::string argv1(argv[1]);
        if (argv1 == "--help") {
            std::cerr << "Usage: " << argv[1] << " [cpuid]\n"
                      << "        print the msr-safe whitelist for host CPU or cpuid if specified in hex on the command line.\n";
        }
        else if (argv1 == "--version") {
            std::cerr << geopm_version();
        }
        else {
            int cpuid = std::stoi(argv1, 0, 16);
            std::cout << geopm::MSRIOGroup::msr_whitelist(cpuid);
        }
    }
    else {
        geopm::MSRIOGroup msriog;
        std::cout <<  msriog.msr_whitelist();
    }
}
