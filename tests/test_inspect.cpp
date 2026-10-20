#include "ps2asset/inspect.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

static std::vector<std::uint8_t> bytes(std::initializer_list<std::uint8_t> init) {
    return std::vector<std::uint8_t>(init);
}

int main() {
    {
        // SL01: offset 0x08 says there are 88 bytes after the fixed 12-byte header.
        auto b = bytes({'S','L','0','1', 0x78,0x56,0x34,0x12, 0x58,0,0,0});
        auto r = ps2asset::inspect_header(b, 100);
        assert(r.is_sl01 && r.sl01_length_checked && r.sl01_length_matches);
        assert(r.format.find("SL01") != std::string::npos);
    }
    {
        auto b = bytes({'S','L','0','1', 0,0,0,0, 0x59,0,0,0});
        auto r = ps2asset::inspect_header(b, 100);
        assert(r.is_sl01 && r.sl01_length_checked && !r.sl01_length_matches);
    }
    {
        auto b = bytes({'T','I','M','2', 4,0, 1,0});
        auto r = ps2asset::inspect_header(b, 9280);
        assert(r.format == "TIM2 texture");
        assert(r.details.size() == 2);
        assert(r.details[0] == "version=4");
        assert(r.details[1] == "picture_count=1");
    }
    {
        auto b = bytes({0,0,1,0xBA});
        auto r = ps2asset::inspect_header(b, 1000);
        assert(r.format == "MPEG program stream (PSS-like video)");
    }
    {
        auto b = bytes({0x7F,'E','L','F', 1,1,1,0});
        auto r = ps2asset::inspect_header(b, 64);
        assert(r.format == "ELF executable/module");
    }
    {
        auto b = bytes({'M','I','G','.','0','0','.','1','P','S','P'});
        auto r = ps2asset::inspect_header(b, 64);
        assert(r.format == "Sony GIM image");
    }
    {
        auto r = ps2asset::inspect_header({}, 0);
        assert(r.format == "unknown");
    }
    std::cout << "All header inspection tests passed.\n";
    return 0;
}
