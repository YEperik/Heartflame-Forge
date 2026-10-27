cmake_minimum_required(VERSION 3.16)
project(HeartflameForge VERSION 1.3.0 LANGUAGES CXX)

# Keep the human-facing VERSION marker in sync with the actual CMake project.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" HEARTFLAME_FORGE_VERSION_FILE LIMIT_COUNT 1)
if(NOT HEARTFLAME_FORGE_VERSION_FILE STREQUAL "${PROJECT_VERSION}")
    message(FATAL_ERROR
        "Version mismatch: VERSION contains '${HEARTFLAME_FORGE_VERSION_FILE}', but CMake project version is '${PROJECT_VERSION}'.")
endif()

include(CTest)
include(GNUInstallDirs)

function(ps2asset_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

option(PS2ASSET_BUILD_GUI "Build the Qt Widgets desktop application when Qt is available" ON)
option(PS2ASSET_REQUIRE_GUI "Fail configuration if Qt Widgets is unavailable when GUI build is requested" OFF)
option(PS2ASSET_BUILD_CLI "Build command-line inspection and texture tools" ON)
option(PS2ASSET_ENABLE_ARCHIVE "Enable browsing TIM2 textures inside ZIP files when libarchive is available" ON)
option(PS2ASSET_BUILD_SAMPLE_PLUGIN "Build a sample dynamic extension when Qt is available" ON)
option(PS2ASSET_BUILD_TOOLS "Build offline validation and maintenance tools" ON)
option(PS2ASSET_ENABLE_PNG "Enable PNG import/export utilities when libpng is available" ON)

# These flags drive the installed package config so exported static-library
# targets find the same optional link dependencies used to build them.
set(PS2ASSET_PACKAGE_NEEDS_LIBARCHIVE OFF)
set(PS2ASSET_PACKAGE_NEEDS_PNG OFF)

# Core library stays independent from Qt: binary readers and format handlers
# can be used by the GUI, CLI, tests, or another application.
add_library(ps2asset_core
    src/inspect.cpp
    src/io/new_file_transaction.cpp
    src/model/inspect.cpp
    src/model/containers.cpp
    src/texture/tim2.cpp
    src/archive/zip_reader.cpp
    src/audio/wav.cpp
    src/text/scan.cpp
    src/workbench/module_catalog.cpp
)
add_library(HeartflameForge::core ALIAS ps2asset_core)
# Deprecated build-tree alias retained for projects migrating from the old name.
add_library(PS2AssetWorkbench::core ALIAS ps2asset_core)
target_include_directories(ps2asset_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(ps2asset_core PUBLIC cxx_std_17)
set_target_properties(ps2asset_core PROPERTIES CXX_EXTENSIONS OFF EXPORT_NAME core)

if(MSVC)
    target_compile_options(ps2asset_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(ps2asset_core PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

if(PS2ASSET_ENABLE_ARCHIVE)
    find_package(LibArchive QUIET)
    if(LibArchive_FOUND)
        if(TARGET LibArchive::LibArchive)
            target_link_libraries(ps2asset_core PRIVATE LibArchive::LibArchive)
        else()
            target_include_directories(ps2asset_core PRIVATE ${LibArchive_INCLUDE_DIRS})
            target_link_libraries(ps2asset_core PRIVATE ${LibArchive_LIBRARIES})
        endif()
        target_compile_definitions(ps2asset_core PRIVATE PS2ASSET_HAS_LIBARCHIVE=1)
        set(PS2ASSET_PACKAGE_NEEDS_LIBARCHIVE ON)
        message(STATUS "ZIP texture browsing: enabled (libarchive)")
    else()
        message(STATUS "ZIP texture browsing: disabled (libarchive development package not found)")
    endif()
endif()

set(PS2ASSET_PNG_AVAILABLE OFF)
if(PS2ASSET_ENABLE_PNG)
    find_package(PNG QUIET)
    if(PNG_FOUND)
        add_library(ps2asset_png src/image/png.cpp)
        add_library(HeartflameForge::png ALIAS ps2asset_png)
        # Deprecated build-tree alias retained for source compatibility.
        add_library(PS2AssetWorkbench::png ALIAS ps2asset_png)
        target_include_directories(ps2asset_png
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
        target_compile_features(ps2asset_png PUBLIC cxx_std_17)
        target_link_libraries(ps2asset_png PUBLIC HeartflameForge::core PRIVATE PNG::PNG)
        set_target_properties(ps2asset_png PROPERTIES CXX_EXTENSIONS OFF EXPORT_NAME png)
        set(PS2ASSET_PACKAGE_NEEDS_PNG ON)
        if(MSVC)
            target_compile_options(ps2asset_png PRIVATE /W4 /permissive-)
        else()
            target_compile_options(ps2asset_png PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
        endif()
        set(PS2ASSET_PNG_AVAILABLE ON)
        message(STATUS "PNG import/export: enabled (libpng ${PNG_VERSION_STRING})")
    else()
        message(STATUS "PNG import/export CLI and PNG integration tests disabled (libpng development package not found)")
    endif()
endif()

if(PS2ASSET_BUILD_CLI)
    add_executable(ps2asset-inspect src/main.cpp)
    target_link_libraries(ps2asset-inspect PRIVATE HeartflameForge::core)

    add_executable(ps2asset-model-inspect src/model_cli.cpp)
    target_link_libraries(ps2asset-model-inspect PRIVATE HeartflameForge::core)

    add_executable(ps2asset-model-extract-sl01 src/model_extract_cli.cpp)
    target_link_libraries(ps2asset-model-extract-sl01 PRIVATE HeartflameForge::core)

    add_executable(ps2asset-text-scan src/text_scan_cli.cpp)
    target_link_libraries(ps2asset-text-scan PRIVATE HeartflameForge::core)

    add_executable(ps2asset-audio-inspect src/audio_inspect_cli.cpp)
    target_link_libraries(ps2asset-audio-inspect PRIVATE HeartflameForge::core)

    add_executable(ps2asset-texture src/texture_cli.cpp)
    target_link_libraries(ps2asset-texture PRIVATE HeartflameForge::core)
    if(PS2ASSET_PNG_AVAILABLE)
        target_link_libraries(ps2asset-texture PRIVATE HeartflameForge::png)
        target_compile_definitions(ps2asset-texture PRIVATE PS2ASSET_HAS_PNG=1)
    endif()
endif()

if(PS2ASSET_BUILD_TOOLS)
    add_executable(ps2asset-texture-validate tools/validate_textures.cpp)
    target_link_libraries(ps2asset-texture-validate PRIVATE HeartflameForge::core)
    if(PS2ASSET_PNG_AVAILABLE)
        target_link_libraries(ps2asset-texture-validate PRIVATE HeartflameForge::png)
        target_compile_definitions(ps2asset-texture-validate PRIVATE PS2ASSET_HAS_PNG=1)
    endif()
    target_compile_features(ps2asset-texture-validate PRIVATE cxx_std_17)
    set_target_properties(ps2asset-texture-validate PROPERTIES CXX_EXTENSIONS OFF)
    if(MSVC)
        target_compile_options(ps2asset-texture-validate PRIVATE /W4 /permissive-)
    else()
        target_compile_options(ps2asset-texture-validate PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
    endif()
endif()

# Assimp is optional. Its GLB exporter only handles input formats that Assimp can import;
# the game's custom .mdl geometry still needs a dedicated decoder.
set(PS2ASSET_ASSIMP_AVAILABLE OFF)
set(PS2ASSET_ASSIMP_TARGET "")
find_package(assimp QUIET CONFIG)
if(TARGET assimp::assimp)
    set(PS2ASSET_ASSIMP_TARGET assimp::assimp)
    set(PS2ASSET_ASSIMP_AVAILABLE ON)
elseif(TARGET assimp)
    set(PS2ASSET_ASSIMP_TARGET assimp)
    set(PS2ASSET_ASSIMP_AVAILABLE ON)
elseif(TARGET Assimp::Assimp)
    set(PS2ASSET_ASSIMP_TARGET Assimp::Assimp)
    set(PS2ASSET_ASSIMP_AVAILABLE ON)
else()
    find_package(Assimp QUIET)
    if(ASSIMP_FOUND)
        set(PS2ASSET_ASSIMP_AVAILABLE ON)
    endif()
endif()
if(PS2ASSET_ASSIMP_AVAILABLE)
    add_executable(ps2asset-model-export-glb src/model_export_glb_cli.cpp)
    if(PS2ASSET_ASSIMP_TARGET)
        target_link_libraries(ps2asset-model-export-glb PRIVATE HeartflameForge::core ${PS2ASSET_ASSIMP_TARGET})
    else()
        target_include_directories(ps2asset-model-export-glb PRIVATE ${ASSIMP_INCLUDE_DIRS})
        target_link_libraries(ps2asset-model-export-glb PRIVATE HeartflameForge::core ${ASSIMP_LIBRARIES})
    endif()
    target_compile_features(ps2asset-model-export-glb PRIVATE cxx_std_17)
    set_target_properties(ps2asset-model-export-glb PROPERTIES CXX_EXTENSIONS OFF)
    message(STATUS "GLB exporter: enabled (Assimp)")
else()
    message(STATUS "GLB exporter: disabled (optional Assimp development package not found)")
endif()

if(PS2ASSET_REQUIRE_GUI AND NOT PS2ASSET_BUILD_GUI)
    message(FATAL_ERROR "PS2ASSET_REQUIRE_GUI=ON but PS2ASSET_BUILD_GUI=OFF. Enable GUI build or disable the strict requirement.")
endif()

if(PS2ASSET_BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Widgets)
    if(Qt6_FOUND)
        set(PS2ASSET_QT_PACKAGE Qt6)
    else()
        find_package(Qt5 5.12 QUIET COMPONENTS Widgets)
        if(Qt5_FOUND)
            set(PS2ASSET_QT_PACKAGE Qt5)
        endif()
    endif()

    if(PS2ASSET_QT_PACKAGE)
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        add_executable(heartflame-forge
            src/gui/main.cpp
            assets/heartflame.qrc
            src/gui/main_window.cpp
            src/gui/main_window.hpp
            src/gui/modules/texture_page.cpp
            src/gui/modules/archive_browser_page.cpp
            src/gui/modules/binary_inspector_page.cpp
            src/gui/modules/model_inspector_page.cpp
            src/gui/plugin_manager.cpp
            src/gui/plugin_manager.hpp
            src/gui/modules/feature_page_base.cpp
            src/gui/modules/feature_page_base.hpp
            src/gui/modules/audio_page.cpp
            src/gui/modules/audio_page.hpp
            src/gui/modules/video_page.cpp
            src/gui/modules/video_page.hpp
            src/gui/modules/animation_page.cpp
            src/gui/modules/animation_page.hpp
            src/gui/modules/text_page.cpp
            src/gui/modules/text_page.hpp
            src/gui/modules/model_export_page.cpp
            src/gui/modules/model_export_page.hpp
            include/ps2asset/workbench/plugin_interface.hpp
        )
        target_link_libraries(heartflame-forge PRIVATE
            HeartflameForge::core
            ${PS2ASSET_QT_PACKAGE}::Widgets
        )
        target_compile_features(heartflame-forge PRIVATE cxx_std_17)
        if(PS2ASSET_ASSIMP_AVAILABLE)
            target_compile_definitions(heartflame-forge PRIVATE PS2ASSET_HAS_ASSIMP=1)
        endif()
        set_target_properties(heartflame-forge PROPERTIES
            CXX_EXTENSIONS OFF
            OUTPUT_NAME "HeartflameForge"
            WIN32_EXECUTABLE ON
        )
        if(WIN32)
            configure_file(
                "${CMAKE_CURRENT_SOURCE_DIR}/cmake/heartflame-forge.rc.in"
                "${CMAKE_CURRENT_BINARY_DIR}/heartflame-forge.rc"
                @ONLY
            )
            target_sources(heartflame-forge PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/heartflame-forge.rc")
            set_property(TARGET heartflame-forge PROPERTY QT_WINDOWS_APP_PROJECT_IDENTIFIER "org.heartflame.forge")
        endif()
        if(MSVC)
            target_compile_options(heartflame-forge PRIVATE /W4 /permissive-)
        else()
            target_compile_options(heartflame-forge PRIVATE -Wall -Wextra -Wpedantic)
        endif()

        if(PS2ASSET_BUILD_SAMPLE_PLUGIN)
            add_library(heartflame_sample_plugin MODULE
                src/gui/plugins/sample_plugin.cpp
                include/ps2asset/workbench/plugin_interface.hpp
            )
            target_include_directories(heartflame_sample_plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
            target_link_libraries(heartflame_sample_plugin PRIVATE ${PS2ASSET_QT_PACKAGE}::Widgets)
            target_compile_features(heartflame_sample_plugin PRIVATE cxx_std_17)
            if(CMAKE_CONFIGURATION_TYPES)
                set(HEARTFLAME_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/$<CONFIG>/plugins")
            else()
                set(HEARTFLAME_PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/plugins")
            endif()
            set_target_properties(heartflame_sample_plugin PROPERTIES
                CXX_EXTENSIONS OFF
                OUTPUT_NAME "heartflame_sample_plugin"
                LIBRARY_OUTPUT_DIRECTORY "${HEARTFLAME_PLUGIN_OUTPUT_DIR}"
                RUNTIME_OUTPUT_DIRECTORY "${HEARTFLAME_PLUGIN_OUTPUT_DIR}"
            )
            target_compile_definitions(heartflame-forge PRIVATE PS2ASSET_EXPECT_SAMPLE_PLUGIN=1)
            install(TARGETS heartflame_sample_plugin
                LIBRARY DESTINATION "plugins"
                RUNTIME DESTINATION "plugins"
            )
        endif()

        message(STATUS "Heartflame Forge desktop GUI: enabled (${PS2ASSET_QT_PACKAGE} Widgets)")
    else()
        if(PS2ASSET_REQUIRE_GUI)
            message(FATAL_ERROR "Qt Widgets development files were not found. Install Qt 6 (qt6-base-dev) or Qt 5 (qtbase5-dev) development packages, then reconfigure. The Heartflame Forge GUI target cannot be built without the Qt headers, CMake package files, and moc tool.")
        else()
            message(WARNING "Qt Widgets development files were not found; the GUI target is skipped. Install Qt 6 or Qt 5 Widgets development packages and reconfigure to build heartflame-forge.")
        endif()
    endif()
endif()

if(BUILD_TESTING AND TARGET heartflame-forge)
    add_test(NAME gui-startup-smoke COMMAND $<TARGET_FILE:heartflame-forge> --smoke-test)
    set_tests_properties(gui-startup-smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()

# Apply consistent baseline diagnostics to all first-party production targets,
# including CLIs that previously relied on compiler defaults.
get_property(PS2ASSET_DIRECTORY_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
foreach(PS2ASSET_TARGET IN LISTS PS2ASSET_DIRECTORY_TARGETS)
    if(PS2ASSET_TARGET MATCHES "^(ps2asset-inspect|ps2asset-model-inspect|ps2asset-model-extract-sl01|ps2asset-text-scan|ps2asset-audio-inspect|ps2asset-texture|ps2asset-texture-validate|ps2asset-model-export-glb|heartflame-forge|heartflame_sample_plugin)$")
        ps2asset_enable_warnings(${PS2ASSET_TARGET})
    endif()
endforeach()

if(BUILD_TESTING)
    if(TARGET ps2asset-model-export-glb)
        add_test(NAME model-export-glb-fixture
            COMMAND ${CMAKE_COMMAND}
                -DTOOL=$<TARGET_FILE:ps2asset-model-export-glb>
                -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/triangle.obj
                -DOUTPUT=${CMAKE_CURRENT_BINARY_DIR}/test_triangle.glb
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_assimp_export.cmake)
    endif()

    add_executable(ps2asset-new-file-transaction-tests tests/test_new_file_transaction.cpp)
    target_link_libraries(ps2asset-new-file-transaction-tests PRIVATE HeartflameForge::core)
    add_test(NAME atomic-new-file-no-clobber COMMAND ps2asset-new-file-transaction-tests)

    add_executable(ps2asset-parser-fuzz-smoke-tests tests/test_parser_fuzz_smoke.cpp)
    target_link_libraries(ps2asset-parser-fuzz-smoke-tests PRIVATE HeartflameForge::core)
    add_test(NAME parser-malformed-input-smoke COMMAND ps2asset-parser-fuzz-smoke-tests)

    add_executable(ps2asset-inspect-tests tests/test_inspect.cpp)
    target_link_libraries(ps2asset-inspect-tests PRIVATE HeartflameForge::core)
    add_test(NAME header-inspection COMMAND ps2asset-inspect-tests)

    add_executable(ps2asset-model-tests tests/test_model.cpp)
    target_link_libraries(ps2asset-model-tests PRIVATE HeartflameForge::core)
    add_test(NAME model-read-only-profile COMMAND ps2asset-model-tests)

    add_executable(ps2asset-model-container-tests tests/test_model_container.cpp)
    target_link_libraries(ps2asset-model-container-tests PRIVATE HeartflameForge::core)
    add_test(NAME model-sl01-container-scan COMMAND ps2asset-model-container-tests)

    add_executable(ps2asset-wav-tests tests/test_wav.cpp)
    target_link_libraries(ps2asset-wav-tests PRIVATE HeartflameForge::core)
    add_test(NAME wav-metadata-parser COMMAND ps2asset-wav-tests)

    add_executable(ps2asset-text-scan-tests tests/test_text_scan.cpp)
    target_link_libraries(ps2asset-text-scan-tests PRIVATE HeartflameForge::core)
    add_test(NAME printable-text-scan COMMAND ps2asset-text-scan-tests)
    if(TARGET ps2asset-text-scan)
        add_test(NAME text-cli-rejects-negative-limit
            COMMAND ps2asset-text-scan ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/plain_strings.bin --max-results -1)
        set_tests_properties(text-cli-rejects-negative-limit PROPERTIES WILL_FAIL TRUE)
        add_test(NAME text-cli-rejects-trailing-junk
            COMMAND ps2asset-text-scan ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/plain_strings.bin --min-length 3junk)
        set_tests_properties(text-cli-rejects-trailing-junk PROPERTIES WILL_FAIL TRUE)
        add_test(NAME text-cli-rejects-signed-number
            COMMAND ps2asset-text-scan ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/plain_strings.bin --min-length +2)
        set_tests_properties(text-cli-rejects-signed-number PROPERTIES WILL_FAIL TRUE)
    endif()

    add_executable(ps2asset-module-catalog-tests tests/test_module_catalog.cpp)
    target_link_libraries(ps2asset-module-catalog-tests PRIVATE HeartflameForge::core)
    add_test(NAME feature-module-catalog COMMAND ps2asset-module-catalog-tests)

    add_executable(ps2asset-tim2-tests tests/test_tim2.cpp)
    target_link_libraries(ps2asset-tim2-tests PRIVATE HeartflameForge::core)
    add_test(NAME tim2-decode-and-replace COMMAND ps2asset-tim2-tests)

    if(PS2ASSET_ENABLE_ARCHIVE AND LibArchive_FOUND)
        add_executable(ps2asset-archive-tests tests/test_archive.cpp)
        if(TARGET LibArchive::LibArchive)
            target_link_libraries(ps2asset-archive-tests PRIVATE HeartflameForge::core LibArchive::LibArchive)
        else()
            target_include_directories(ps2asset-archive-tests PRIVATE ${LibArchive_INCLUDE_DIRS})
            target_link_libraries(ps2asset-archive-tests PRIVATE HeartflameForge::core ${LibArchive_LIBRARIES})
        endif()
        add_test(NAME zip-archive-listing COMMAND ps2asset-archive-tests)
    endif()
    if(TARGET ps2asset-texture-validate)
        add_test(NAME texture-replacement-roundtrip-self-test COMMAND ps2asset-texture-validate --self-test)
        add_test(NAME texture-validator-reports-unsupported-profile
            COMMAND ${CMAKE_COMMAND}
                -DTOOL=$<TARGET_FILE:ps2asset-texture-validate>
                -DINPUT=${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/tim2_indexed_psm0.tm2
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_unsupported_texture_validator.cmake)
    endif()
    if(PS2ASSET_PNG_AVAILABLE)
        add_executable(ps2asset-png-tests tests/test_png.cpp)
        target_link_libraries(ps2asset-png-tests PRIVATE HeartflameForge::png)
        add_test(NAME png-rgba-roundtrip COMMAND ps2asset-png-tests)
    endif()
    get_property(PS2ASSET_TEST_TARGETS DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(PS2ASSET_TARGET IN LISTS PS2ASSET_TEST_TARGETS)
        if(PS2ASSET_TARGET MATCHES "-tests$")
            ps2asset_enable_warnings(${PS2ASSET_TARGET})
        endif()
    endforeach()
endif()

install(TARGETS ps2asset_core
    EXPORT HeartflameForgeTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(TARGET ps2asset-inspect)
    install(TARGETS ps2asset-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-model-inspect)
    install(TARGETS ps2asset-model-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-model-extract-sl01)
    install(TARGETS ps2asset-model-extract-sl01 RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-text-scan)
    install(TARGETS ps2asset-text-scan RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-audio-inspect)
    install(TARGETS ps2asset-audio-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-model-export-glb)
    if(TARGET heartflame-forge)
        # The GUI starts this helper by looking beside HeartflameForge.exe.
        install(TARGETS ps2asset-model-export-glb RUNTIME DESTINATION .)
    else()
        install(TARGETS ps2asset-model-export-glb RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    endif()
endif()
if(TARGET ps2asset-texture)
    install(TARGETS ps2asset-texture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-texture-validate)
    install(TARGETS ps2asset-texture-validate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset_png)
    install(TARGETS ps2asset_png EXPORT HeartflameForgeTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
if(TARGET heartflame-forge)
    # GUI releases are portable: the executable lives at the package root.
    install(TARGETS heartflame-forge RUNTIME DESTINATION .)
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install a consumable CMake package for downstream C++ tools. The generated
# config discovers optional link dependencies only when those were used to
# build the installed libraries.
include(CMakePackageConfigHelpers)
set(HEARTFLAME_FORGE_PACKAGE_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/HeartflameForge")
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/HeartflameForgeConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HeartflameForgeConfig.cmake"
    INSTALL_DESTINATION "${HEARTFLAME_FORGE_PACKAGE_INSTALL_DIR}"
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HeartflameForgeConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)
install(EXPORT HeartflameForgeTargets
    FILE HeartflameForgeTargets.cmake
    NAMESPACE HeartflameForge::
    DESTINATION "${HEARTFLAME_FORGE_PACKAGE_INSTALL_DIR}"
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HeartflameForgeConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HeartflameForgeConfigVersion.cmake"
    DESTINATION "${HEARTFLAME_FORGE_PACKAGE_INSTALL_DIR}"
)
