#include "ps2asset/workbench/module_catalog.hpp"

#include <algorithm>

namespace ps2asset::workbench {

const std::vector<ModuleDescriptor>& builtin_modules() {
    static const std::vector<ModuleDescriptor> modules = {
        {"dashboard", "Ikhtisar", "Workspace", "Ringkasan proyek dan status parser.", ModuleReadiness::Implemented,
         {}, {"Navigasi modul", "Status fitur"}, {}},
        {"texture", "Texture Workshop", "Graphics", "Pratinjau, import/export PNG, dan TIM2 replacement.", ModuleReadiness::Implemented,
         {".tm2", ".png", ".zip"}, {"TIM2 decode untuk profil tervalidasi", "PNG import/export", "Batch ZIP replacement"},
         {"TIM2 profile selain indexed 8-bit + RGBA32 CLUT"}},
        {"archive", "Archive Browser", "Containers", "Daftar entry ZIP dan operasi replacement aman.", ModuleReadiness::Implemented,
         {".zip"}, {"Pencarian entry", "Batch TIM2 replacement ke ZIP baru"}, {"Rebuild tipe arsip selain ZIP"}},
        {"binary", "Binary Inspector", "Reverse Engineering", "Header, signature, dan hex preview read-only.", ModuleReadiness::Implemented,
         {"binary"}, {"Signature sniffing", "Hex preview", "ASCII search"}, {"Interpretasi semantik field yang belum dikenal"}},
        {"model-inspector", "Model Inspector", "3D", "Profil MDL dan scan container SL01.", ModuleReadiness::ReadOnlyResearch,
         {".mdl"}, {"Header/marker inspection", "SL01 candidate bounds check"}, {"Vertex, indeks, UV, material, transform, skeleton"}},
        {"model-export-glb", "Model Export (GLB)", "3D", "Ekspor format standar via Assimp bila dependency tersedia.", ModuleReadiness::OptionalDependency,
         {"Format impor Assimp"}, {"Target CLI opsional jika Assimp ditemukan"}, {"Decoder geometri khusus game .mdl sebelum ekspor GLB"}},
        {"animation", "Animation Inspector", "Animation", "Workspace terpisah untuk inspeksi container dan riset track animasi.", ModuleReadiness::BlockedOnFormatResearch,
         {".mdl", "binary"}, {"Read-only signature and range inspection"}, {"Keyframe layout, time base, bone hierarchy, interpolation"}},
        {"audio", "Audio Inspector", "Media", "Inspeksi metadata RIFF/WAVE dan signature resource proprietary.", ModuleReadiness::Implemented,
         {".wav", "SDPK", "ADSC"}, {"WAV chunk bounds/metadata parser", "Header detection"}, {"Codec internal SDPK/ADSC dan playback"}},
        {"video", "Video / Stream Inspector", "Media", "Deteksi signature container/video tanpa menganggapnya decoder.", ModuleReadiness::ReadOnlyResearch,
         {"PSS-like MPEG stream", "binary"}, {"MPEG pack-start detection", "Generic header profile"}, {"Validasi PSS block layout, demux, codec profile"}},
        {"text", "Text & Script Inspector", "Data", "Pencarian string ASCII dan subset UTF-16LE dengan offset.", ModuleReadiness::Implemented,
         {"binary", "ASCII", "UTF-16LE ASCII subset"}, {"Bounded string extraction", "TSV export via CLI"}, {"Encoding, relocation, string table, script opcode"}},
    };
    return modules;
}

const ModuleDescriptor* find_builtin_module(const std::string& id) {
    const auto& modules = builtin_modules();
    const auto it = std::find_if(modules.begin(), modules.end(), [&id](const ModuleDescriptor& module) {
        return module.id == id;
    });
    return it == modules.end() ? nullptr : &*it;
}

std::string readiness_name(ModuleReadiness readiness) {
    switch (readiness) {
    case ModuleReadiness::Implemented: return "implemented";
    case ModuleReadiness::ReadOnlyResearch: return "read-only research";
    case ModuleReadiness::OptionalDependency: return "optional dependency";
    case ModuleReadiness::BlockedOnFormatResearch: return "blocked on format research";
    }
    return "unknown";
}

}  // namespace ps2asset::workbench
