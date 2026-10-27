#include "ps2asset/text/scan.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

int main() {
    {
        const std::vector<std::uint8_t> b{
            0x00U, 'H','E','L','L','O',0xFFU,0x00U, 0x54U,0x00U,0x45U,0x00U,0x53U,0x00U,0x54U,0x00U,
            0x00U, 'x','y'
        };
        const auto r = ps2asset::text::scan_printable_strings(b, 4U, 10U);
        assert(r.bytes_scanned == b.size());
        assert(r.hits.size() >= 2U);
        bool foundAscii = false;
        bool foundUtf16 = false;
        for (const auto& hit : r.hits) {
            if (hit.encoding == ps2asset::text::StringEncoding::ASCII && hit.value == "HELLO" && hit.offset == 1U) foundAscii = true;
            if (hit.encoding == ps2asset::text::StringEncoding::UTF16LE_ASCII_SUBSET && hit.value == "TEST" && hit.offset == 8U) foundUtf16 = true;
        }
        assert(foundAscii && foundUtf16);
    }
    {
        const std::vector<std::uint8_t> b{'A','B','C','D',0,'E','F','G','H'};
        const auto r = ps2asset::text::scan_printable_strings(b, 4U, 1U);
        assert(r.hits.size() == 1U);
        assert(r.truncated_by_limit);
    }
    {
        bool threw = false;
        try { (void)ps2asset::text::scan_printable_strings({}, 1U, 10U); }
        catch (const std::invalid_argument&) { threw = true; }
        assert(threw);
    }
    std::cout << "All printable text scan tests passed.\n";
    return 0;
}
