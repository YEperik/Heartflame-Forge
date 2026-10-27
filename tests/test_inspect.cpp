#include "ps2asset/inspect.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
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
        auto b = bytes({'T','I','M','2', 4,5, 1,0});
        auto r = ps2asset::inspect_header(b, 9280);
        assert(r.format == "TIM2 texture");
        assert(r.details.size() == 3);
        assert(r.details[0] == "version=4");
        assert(r.details[1] == "format_type=5");
        assert(r.details[2] == "picture_count=1");
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
        std::vector<std::uint8_t> b(48U, 0U);
        b[0] = 'A'; b[1] = 'D'; b[2] = 'S'; b[3] = 'C';
        b[8] = 'S'; b[9] = 'S'; b[10] = 'h'; b[11] = 'd';
        b[12] = 24U;
        b[40] = 'S'; b[41] = 'S'; b[42] = 'b'; b[43] = 'd';
        b[44] = 64U;
        const auto r = ps2asset::inspect_header(b, 1000U);
        assert(r.format.find("ADSC") != std::string::npos);
        assert(std::find(r.details.begin(), r.details.end(), "second chunk tag=SSbd at 0x28") != r.details.end());
        assert(std::find(r.details.begin(), r.details.end(), "raw field@0x2C=64 (semantics unknown)") != r.details.end());
        assert(std::find(r.details.begin(), r.details.end(), "file_size minus raw field@0x2C=936 (arithmetic only; not a decoded payload length)") != r.details.end());
    }
    {
        auto b = bytes({'S','D','P','K'});
        b.resize(40U, 0U);
        const std::string observed = "IECSsreV";
        std::copy(observed.begin(), observed.end(), b.begin() + 32);
        const auto r = ps2asset::inspect_header(b, b.size());
        assert(std::find(r.details.begin(), r.details.end(), "raw bytes@0x20=IECSsreV (observed marker; semantics unknown)") != r.details.end());
    }
    {
        auto r = ps2asset::inspect_header({}, 0);
        assert(r.format == "unknown");
    }
    std::cout << "All header inspection tests passed.\n";
    return 0;
}
