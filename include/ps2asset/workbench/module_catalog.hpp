#pragma once

#include <string>
#include <vector>

namespace ps2asset::workbench {

enum class ModuleReadiness {
    Implemented,
    ReadOnlyResearch,
    OptionalDependency,
    BlockedOnFormatResearch
};

struct ModuleDescriptor {
    std::string id;
    std::string display_name;
    std::string category;
    std::string summary;
    ModuleReadiness readiness = ModuleReadiness::ReadOnlyResearch;
    std::vector<std::string> supported_inputs;
    std::vector<std::string> implemented_capabilities;
    std::vector<std::string> missing_analysis;
};

// Single source of truth for tool inventory/status. Descriptors deliberately
// distinguish implemented behavior from research targets.
const std::vector<ModuleDescriptor>& builtin_modules();
const ModuleDescriptor* find_builtin_module(const std::string& id);
std::string readiness_name(ModuleReadiness readiness);

}  // namespace ps2asset::workbench
