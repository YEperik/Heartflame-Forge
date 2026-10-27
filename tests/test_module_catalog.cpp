#include "ps2asset/workbench/module_catalog.hpp"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <set>

int main() {
    const auto& modules = ps2asset::workbench::builtin_modules();
    assert(modules.size() >= 10U);
    std::set<std::string> ids;
    for (const auto& module : modules) {
        assert(!module.id.empty());
        assert(!module.display_name.empty());
        assert(ids.insert(module.id).second);
        assert(!module.summary.empty());
    }
    const auto* text = ps2asset::workbench::find_builtin_module("text");
    assert(text != nullptr);
    assert(text->readiness == ps2asset::workbench::ModuleReadiness::Implemented);
    const auto* glb = ps2asset::workbench::find_builtin_module("model-export-glb");
    assert(glb != nullptr);
    assert(glb->readiness == ps2asset::workbench::ModuleReadiness::OptionalDependency);
    assert(!glb->missing_analysis.empty());
    assert(ps2asset::workbench::find_builtin_module("not-real") == nullptr);
    assert(ps2asset::workbench::readiness_name(ps2asset::workbench::ModuleReadiness::BlockedOnFormatResearch) == "blocked on format research");
    std::cout << "All module catalog tests passed.\n";
    return 0;
}
