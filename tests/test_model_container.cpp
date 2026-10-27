#include "ps2asset/model/containers.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

void put_u32_le(std::vector<std::uint8_t>& bytes, std::size_t off,
                std::uint32_t value) {
    bytes[off] = static_cast<std::uint8_t>(value & 0xFFU);
    bytes[off + 1U] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    bytes[off + 2U] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    bytes[off + 3U] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

void put_sl01(std::vector<std::uint8_t>& bytes, std::size_t off,
              std::uint32_t payload_size) {
    bytes[off] = 'S';
    bytes[off + 1U] = 'L';
    bytes[off + 2U] = '0';
    bytes[off + 3U] = '1';
    put_u32_le(bytes, off + 4U, 0x12345678U);  // Unknown field; kept raw.
    put_u32_le(bytes, off + 8U, payload_size);
}

}  // namespace

int main() {
    {
        // A standard wrapper occupies the whole file. An incidental SL01 byte
        // pattern in its body must not be mistaken for a second block.
        std::vector<std::uint8_t> bytes(100U, 0xA5U);
        put_sl01(bytes, 0U, 88U);
        bytes[30] = 'S'; bytes[31] = 'L'; bytes[32] = '0'; bytes[33] = '1';
        const auto result = ps2asset::model::scan_embedded_sl01_blocks(bytes);
        assert(result.signature_hits == 2U);
        assert(result.blocks.size() == 1U);
        assert(result.blocks[0].offset == 0U);
        assert(result.blocks[0].total_size == 100U);
        assert(result.rejected_candidates == 0U);
    }
    {
        // Two embedded blocks are separated by five padding bytes.
        constexpr std::size_t first_offset = 16U;
        constexpr std::size_t first_total = 32U;
        constexpr std::size_t padding = 5U;
        constexpr std::size_t second_offset = first_offset + first_total + padding;
        constexpr std::size_t second_total = 24U;
        std::vector<std::uint8_t> bytes(second_offset + second_total, 0xEEU);
        put_sl01(bytes, first_offset, 20U);
        put_sl01(bytes, second_offset, 12U);

        const auto result = ps2asset::model::scan_embedded_sl01_blocks(bytes);
        assert(result.blocks.size() == 2U);
        assert(result.blocks[0].offset == first_offset);
        assert(result.blocks[0].total_size == first_total);
        assert(result.blocks[0].gap_after == padding);
        assert(result.blocks[1].offset == second_offset);
        assert(result.blocks[1].total_size == second_total);
        assert(result.blocks[1].gap_after == 0U);
    }
    {
        std::vector<std::uint8_t> bytes(32U, 0U);
        bytes[5] = 'S'; bytes[6] = 'L'; bytes[7] = '0'; bytes[8] = '1';
        put_u32_le(bytes, 13U, 0xFFFFFFF0U);
        const auto result = ps2asset::model::scan_embedded_sl01_blocks(bytes);
        assert(result.signature_hits == 1U);
        assert(result.blocks.empty());
        assert(result.rejected_candidates == 1U);
        assert(!result.warnings.empty());
    }
    {
        const auto result = ps2asset::model::scan_embedded_sl01_blocks({});
        assert(result.source_size == 0U);
        assert(result.blocks.empty());
        assert(result.signature_hits == 0U);
    }

    {
        // Bound memory spent on marker offsets for a hostile/repetitive buffer.
        std::vector<std::uint8_t> bytes(400004U, 0U);
        for (std::size_t off = 0; off + 4U <= bytes.size(); off += 4U) {
            bytes[off] = 'S'; bytes[off + 1U] = 'L';
            bytes[off + 2U] = '0'; bytes[off + 3U] = '1';
        }
        const auto result = ps2asset::model::scan_embedded_sl01_blocks(bytes);
        assert(result.scan_aborted_hit_limit);
        assert(result.blocks.empty());
        assert(result.signature_hits == 100000U);
        assert(!result.warnings.empty());
    }

    std::cout << "All SL01 container scan tests passed.\n";
    return 0;
}
