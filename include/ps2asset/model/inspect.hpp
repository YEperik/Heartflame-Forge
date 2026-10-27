#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ps2asset::model {

struct MarkerSummary {
    std::string marker;
    std::uint64_t count = 0;
    std::vector<std::uint64_t> first_offsets;
};

struct ModelInspectionResult {
    std::string name;
    std::uint64_t file_size = 0;
    std::string format;
    bool sl01_wrapped = false;
    bool sl01_length_checked = false;
    bool sl01_length_matches = false;
    std::uint32_t sl01_field04 = 0;
    std::uint32_t sl01_field08 = 0;
    std::vector<MarkerSummary> markers;
    std::vector<std::string> embedded_path_candidates;
    std::vector<std::string> warnings;
};

// Read-only heuristic profiling of an MDL candidate. Marker names and embedded
// path strings are reported as byte-level evidence, not decoded semantics.
// The caller owns file I/O and should apply a size limit before allocating bytes.
ModelInspectionResult inspect_model_bytes(const std::vector<std::uint8_t>& bytes,
                                          const std::string& name = {});

// Formats the inspection result as a human-readable text report.
std::string format_model_report(const ModelInspectionResult& result);

}  // namespace ps2asset::model
