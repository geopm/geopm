#include <cstdint>

#include <iostream>
#include <vector>

#include "geopm_hash.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    std::vector<char> input(data, data + size);
    input.push_back('\0');
    uint64_t result = geopm_crc32_str(input.data());
    std::cout << "[DEBUG] " << result << std::endl;
    return 0;
}
