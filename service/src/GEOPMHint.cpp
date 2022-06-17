#include "geopm_hint.h"

#include <string>

#include "geopm/Helper.hpp"
#include "geopm/Exception.hpp"

extern "C" {

    void check_hint(uint64_t hint)
    {
        if ((hint & ~GEOPM_MASK_REGION_HINT) != 0ULL) {
            throw geopm::Exception("Helper::" + std::string(__func__) +
                            "(): invalid hint: " +
                            geopm::string_format_hex(hint),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
        if (hint >= GEOPM_SENTINEL_REGION_HINT) {
            throw geopm::Exception("Helper::" + std::string(__func__) +
                            "(): hint out of range: " +
                            geopm::string_format_hex(hint),
                            GEOPM_ERROR_INVALID, __FILE__, __LINE__);
        }
    }

}
