#include "ps2asset/model/inspect.hpp"
#include "ps2asset/model/containers.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <utility>

namespace ps2asset::model {
namespace {

std::uint32_t u32le(const std::vector<std::uint8_t>& bytes, std::size_t offset) {
    return static_cast<std::uint32_t>(bytes[offset]) |
           (static_cast<std::uint32_t>(bytes[offset + 1]) << 8U) |
           (static_cast<std::uint32_t>(bytes[offset + 2]) << 16U) |
           (static_cast<std::uint32_t>(bytes[offset + 3]) << 24U);
}

bool starts_with(const std::vector<std::uint8_t>& bytes, std::string_view value) {
    if (bytes.size() < value.size()) return false;
    return std::equal(value.begin(), value.end(), bytes.begin(),
        [](char a, std::uint8_t b) { return static_cast<unsigned char>(a) == b; });
}

std::string hex_u32(std::uint32_t value) {
    std::ostringstream out;
    out << "0x" << std::uppercase << std::hex << std::setw(8)
        << std::setfill('0') << value;
    return out.str();
}

bool has_mdl_extension(const std::string& name) {
    const auto dot = name.find_last_of('.');
    if (dot == std::string::npos) return false;
    std::string extension = name.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return extension == ".mdl";
}

bool printable_ascii(std::uint8_t value) {
    return value >= 0x20U && value <= 0x7EU;
}

bool looks_like_path_candidate(const std::string& value) {
    if (value.find('/') == std::string::npos && value.find('\\') == std::string::npos) {
        return false;
    }
    // The path-like separator is direct byte evidence; extensions merely help
    // prioritize likely resource references and are not required for reporting.
    const std::string lower = [&value]() {
        std::string copy = value;
        std::transform(copy.begin(), copy.end(), copy.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return copy;
    }();
    return lower.find(".mdl") != std::string::npos ||
           lower.find(".map") != std::string::npos ||
           lower.find(".tm2") != std::string::npos ||
           lower.find(".gim") != std::string::npos ||
           lower.find(".atb") != std::string::npos ||
           lower.find("character/") != std::string::npos ||
           lower.find("/system/") != std::string::npos ||
           lower.find("system/") != std::string::npos;
}

std::vector<MarkerSummary> scan_markers(const std::vector<std::uint8_t>& bytes) {
    // These are four-byte ASCII patterns observed in the supplied asset set.
    // Their precise roles and enclosing record lengths are not yet established.
    constexpr std::array<std::string_view, 13> marker_names{{
        "MDHD", "MDMT", "MDOB", "MDCC", "MDST",
        "FHDR", "FMAP", "FOPT", "FETC", "FATB",
        "FNAM", "FMDL", "FTEX"
    }};
    std::vector<MarkerSummary> summaries;
    summaries.reserve(marker_names.size());
    for (const std::string_view marker : marker_names) {
        MarkerSummary summary;
        summary.marker = std::string(marker);
        if (bytes.size() >= marker.size()) {
            auto begin = bytes.begin();
            while (begin != bytes.end()) {
                const auto found = std::search(begin, bytes.end(), marker.begin(), marker.end(),
                    [](std::uint8_t byte, char ch) {
                        return byte == static_cast<std::uint8_t>(static_cast<unsigned char>(ch));
                    });
                if (found == bytes.end()) break;
                const auto offset = static_cast<std::uint64_t>(std::distance(bytes.begin(), found));
                ++summary.count;
                if (summary.first_offsets.size() < 8U) summary.first_offsets.push_back(offset);
                begin = found + 1;
            }
        }
        if (summary.count > 0U) summaries.push_back(std::move(summary));
    }
    return summaries;
}

std::vector<std::string> scan_path_candidates(const std::vector<std::uint8_t>& bytes) {
    std::vector<std::string> found;
    std::size_t cursor = 0;
    constexpr std::size_t min_length = 5U;
    constexpr std::size_t max_length = 160U;
    constexpr std::size_t max_candidates = 64U;
    while (cursor < bytes.size() && found.size() < max_candidates) {
        while (cursor < bytes.size() && !printable_ascii(bytes[cursor])) ++cursor;
        const std::size_t start = cursor;
        while (cursor < bytes.size() && printable_ascii(bytes[cursor])) {
            ++cursor;
        }
        const std::size_t length = cursor - start;
        if (length >= min_length && length <= max_length) {
            std::string candidate;
            candidate.reserve(length);
            for (std::size_t i = start; i < cursor; ++i) {
                candidate.push_back(static_cast<char>(bytes[i]));
            }
            if (looks_like_path_candidate(candidate) &&
                std::find(found.begin(), found.end(), candidate) == found.end()) {
                found.push_back(std::move(candidate));
            }
        }
        // If a printable run exceeded the cap, cursor already points near its
        // first nonprintable byte; the next iteration advances over it safely.
    }
    return found;
}

}  // namespace

ModelInspectionResult inspect_model_bytes(const std::vector<std::uint8_t>& bytes,
                                          const std::string& name) {
    ModelInspectionResult result;
    result.name = name.empty() ? "(memory buffer)" : name;
    result.file_size = static_cast<std::uint64_t>(bytes.size());

    if (bytes.empty()) {
        result.format = "Empty model candidate";
        result.warnings.emplace_back("The input contains no bytes.");
        return result;
    }

    if (starts_with(bytes, "SL01")) {
        result.sl01_wrapped = true;
        result.format = "SL01-wrapped MDL candidate";
        if (bytes.size() < 12U) {
            result.warnings.emplace_back("Truncated SL01 prefix: fewer than 12 bytes.");
        } else {
            result.sl01_field04 = u32le(bytes, 4U);
            result.sl01_field08 = u32le(bytes, 8U);
            result.sl01_length_checked = true;
            const std::uint64_t bytes_after_prefix =
                static_cast<std::uint64_t>(bytes.size() - 12U);
            result.sl01_length_matches =
                static_cast<std::uint64_t>(result.sl01_field08) == bytes_after_prefix;
            if (!result.sl01_length_matches) {
                result.warnings.emplace_back(
                    "SL01 field@0x08 does not match file size minus 12 bytes.");
            }
            result.warnings.emplace_back(
                "SL01 field@0x04 is shown as raw data; its meaning is unknown.");
        }
    } else if (bytes.size() >= 4U) {
        const std::uint32_t lead = u32le(bytes, 0U);
        if (lead == 3U || lead == 0x16U || lead == 0x48U) {
            result.format = "Unwrapped MDL-like header (" + hex_u32(lead) + ")";
            result.warnings.emplace_back(
                "Leading word is a pattern from the supplied .mdl inventory; field semantics are unknown.");
        } else if (has_mdl_extension(name)) {
            result.format = "Unclassified .mdl candidate";
            result.warnings.emplace_back(
                "File extension suggests MDL, but no supported header pattern was recognized.");
        } else {
            result.format = "Unknown model/resource candidate";
            result.warnings.emplace_back("No recognized MDL header pattern was found.");
        }
    } else {
        result.format = "Truncated model candidate";
        result.warnings.emplace_back("Fewer than four bytes are available for header classification.");
    }

    const auto sl01_scan = scan_embedded_sl01_blocks(bytes);
    result.validated_sl01_blocks = static_cast<std::uint64_t>(sl01_scan.blocks.size());
    result.sl01_signature_hits = sl01_scan.signature_hits;
    result.sl01_rejected_candidates = sl01_scan.rejected_candidates;
    result.sl01_scan_aborted_hit_limit = sl01_scan.scan_aborted_hit_limit;
    if (sl01_scan.scan_aborted_hit_limit) {
        result.warnings.emplace_back(
            "SL01 range scan stopped after the 100000-signature safety limit; no ranges were accepted from that scan.");
    } else if (sl01_scan.signature_hits > 0U && sl01_scan.blocks.empty()) {
        result.warnings.emplace_back(
            "SL01 signatures were found, but no candidate range passed bounds/overlap checks.");
    }
    for (std::size_t i = 0; i < sl01_scan.blocks.size() && i < 8U; ++i) {
        result.first_sl01_block_offsets.push_back(sl01_scan.blocks[i].offset);
    }

    result.markers = scan_markers(bytes);
    result.embedded_path_candidates = scan_path_candidates(bytes);
    if (result.markers.empty()) {
        result.warnings.emplace_back(
            "No known four-byte marker patterns were found; this does not prove the asset lacks model data.");
    } else {
        result.warnings.emplace_back(
            "Marker hits are byte-pattern evidence only; chunk boundaries and semantics are not decoded.");
    }
    if (result.embedded_path_candidates.empty()) {
        result.warnings.emplace_back("No path-like ASCII candidates were found in the scanned bytes.");
    } else {
        result.warnings.emplace_back(
            "Embedded path candidates are printable byte strings and may be names or incidental data.");
    }
    return result;
}

std::string format_model_report(const ModelInspectionResult& result) {
    std::ostringstream out;
    out << "PS2 Asset Workbench — Read-only MDL Profile\n"
        << "===========================================\n"
        << "Name: " << result.name << '\n'
        << "Size: " << result.file_size << " bytes\n"
        << "Classification: " << result.format << '\n';
    if (result.sl01_wrapped) {
        out << "SL01 wrapper: yes\n";
        if (result.sl01_length_checked) {
            out << "SL01 field@0x04: " << hex_u32(result.sl01_field04)
                << " (semantics unknown)\n"
                << "SL01 field@0x08: " << result.sl01_field08 << '\n'
                << "SL01 size invariant (size - 12): "
                << (result.sl01_length_matches ? "MATCH" : "MISMATCH") << '\n';
        } else {
            out << "SL01 size invariant: NOT CHECKED (truncated header)\n";
        }
    } else {
        out << "SL01 wrapper: no\n";
    }

    out << "\nValidated SL01 byte ranges (field@0x08 bounds check only)\n"
        << "  block_count=" << result.validated_sl01_blocks
        << " signature_hits=" << result.sl01_signature_hits
        << " rejected_candidates=" << result.sl01_rejected_candidates
        << " hit_limit_exceeded=" << (result.sl01_scan_aborted_hit_limit ? "yes" : "no")
        << " offsets=";
    if (result.first_sl01_block_offsets.empty()) {
        out << "(none)";
    } else {
        for (std::size_t i = 0; i < result.first_sl01_block_offsets.size(); ++i) {
            if (i) out << ", ";
            out << "0x" << std::uppercase << std::hex
                << result.first_sl01_block_offsets[i] << std::dec;
        }
        if (result.validated_sl01_blocks > result.first_sl01_block_offsets.size()) out << ", ...";
    }
    out << "\n";

    out << "\nObserved four-byte markers (counts + first offsets)\n";
    if (result.markers.empty()) {
        out << "  (none)\n";
    }
    for (const auto& marker : result.markers) {
        out << "  " << marker.marker << " count=" << marker.count << " offsets=";
        for (std::size_t i = 0; i < marker.first_offsets.size(); ++i) {
            if (i) out << ", ";
            out << "0x" << std::uppercase << std::hex << marker.first_offsets[i] << std::dec;
        }
        if (marker.count > marker.first_offsets.size()) out << ", ...";
        out << '\n';
    }

    out << "\nEmbedded path-like ASCII candidates (not decoded references)\n";
    if (result.embedded_path_candidates.empty()) out << "  (none)\n";
    for (const auto& value : result.embedded_path_candidates) out << "  " << value << '\n';

    out << "\nCautions\n";
    if (result.warnings.empty()) out << "  (none)\n";
    for (const auto& warning : result.warnings) out << "  - " << warning << '\n';
    out << "\nNo mesh, vertex, material, skeleton, or animation decoding is performed by this profile.\n";
    return out.str();
}

}  // namespace ps2asset::model
