#include "ps2asset/model/inspect.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

int main() {
    {
        std::vector<std::uint8_t> bytes(100U, 0U);
        bytes[0] = 'S'; bytes[1] = 'L'; bytes[2] = '0'; bytes[3] = '1';
        bytes[4] = 0x78U; bytes[5] = 0x56U; bytes[6] = 0x34U; bytes[7] = 0x12U;
        bytes[8] = 88U;
        bytes[20] = 'M'; bytes[21] = 'D'; bytes[22] = 'H'; bytes[23] = 'D';
        bytes[32] = 'F'; bytes[33] = 'T'; bytes[34] = 'E'; bytes[35] = 'X';
        const auto r = ps2asset::model::inspect_model_bytes(bytes, "fixture.mdl");
        assert(r.format == "SL01-wrapped MDL candidate");
        assert(r.sl01_wrapped && r.sl01_length_checked && r.sl01_length_matches);
        assert(r.sl01_field04 == 0x12345678U);
        assert(r.sl01_field08 == 88U);
        assert(r.markers.size() == 2U);
        assert(r.markers[0].marker == "MDHD" && r.markers[0].count == 1U);
        assert(r.markers[0].first_offsets.front() == 20U);
        assert(r.markers[1].marker == "FTEX");
    }
    {
        std::vector<std::uint8_t> bytes(20U, 0U);
        bytes[0] = 'S'; bytes[1] = 'L'; bytes[2] = '0'; bytes[3] = '1';
        bytes[8] = 99U;
        const auto r = ps2asset::model::inspect_model_bytes(bytes, "broken.mdl");
        assert(r.sl01_length_checked && !r.sl01_length_matches);
        assert(!r.warnings.empty());
        assert(ps2asset::model::format_model_report(r).find("MISMATCH") != std::string::npos);
    }
    {
        const std::vector<std::uint8_t> bytes{
            0x03U, 0x00U, 0x00U, 0x00U, 0x20U, 0x00U, 0x20U, 0x00U
        };
        const auto r = ps2asset::model::inspect_model_bytes(bytes, "hit_ito.mdl");
        assert(r.format.find("Unwrapped MDL-like header") == 0U);
        assert(!r.sl01_wrapped);
    }
    {
        std::vector<std::uint8_t> bytes{
            'S','L','0','1', 0U,0U,0U,0U, 0U,0U,0U,0U
        };
        const std::string path = "/system/ichigo01e_g.map";
        bytes.insert(bytes.end(), path.begin(), path.end());
        bytes.push_back(0U);
        const auto r = ps2asset::model::inspect_model_bytes(bytes, "embedded.mdl");
        assert(!r.embedded_path_candidates.empty());
        assert(std::find(r.embedded_path_candidates.begin(), r.embedded_path_candidates.end(), path) !=
               r.embedded_path_candidates.end());
    }
    {
        const auto empty = ps2asset::model::inspect_model_bytes({}, "empty.mdl");
        assert(empty.format == "Empty model candidate");
        const auto tiny = ps2asset::model::inspect_model_bytes({0x53U, 0x4CU}, "tiny.mdl");
        assert(tiny.format == "Truncated model candidate");
    }
    {
        const auto unknown = ps2asset::model::inspect_model_bytes(
            {0xFFU, 0x11U, 0x22U, 0x33U}, "random.bin");
        assert(unknown.format == "Unknown model/resource candidate");
    }

    std::cout << "All model inspection tests passed.\n";
    return 0;
}
