cmake_minimum_required(VERSION 3.16)
project(PS2AssetWorkbench VERSION 0.5.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(PS2ASSET_BUILD_GUI "Build the Qt Widgets desktop application when Qt is available" ON)
option(PS2ASSET_BUILD_CLI "Build command-line inspection and texture tools" ON)
option(PS2ASSET_ENABLE_ARCHIVE "Enable browsing TIM2 textures inside ZIP files when libarchive is available" ON)
option(PS2ASSET_BUILD_SAMPLE_PLUGIN "Build a sample dynamic extension when Qt is available" ON)
option(PS2ASSET_BUILD_TOOLS "Build offline validation and maintenance tools" ON)
option(PS2ASSET_ENABLE_PNG "Enable PNG import/export utilities when libpng is available" ON)

# Core library stays independent from Qt: binary readers and format handlers
# can be used by the GUI, CLI, tests, or another application.
add_library(ps2asset_core
    src/inspect.cpp
    src/texture/tim2.cpp
    src/archive/zip_reader.cpp
)
add_library(PS2AssetWorkbench::core ALIAS ps2asset_core)
target_include_directories(ps2asset_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(ps2asset_core PUBLIC cxx_std_17)
set_target_properties(ps2asset_core PROPERTIES CXX_EXTENSIONS OFF)

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
        add_library(PS2AssetWorkbench::png ALIAS ps2asset_png)
        target_include_directories(ps2asset_png
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
        )
        target_compile_features(ps2asset_png PUBLIC cxx_std_17)
        target_link_libraries(ps2asset_png PUBLIC PS2AssetWorkbench::core PRIVATE PNG::PNG)
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
    target_link_libraries(ps2asset-inspect PRIVATE PS2AssetWorkbench::core)

    add_executable(ps2asset-texture src/texture_cli.cpp)
    target_link_libraries(ps2asset-texture PRIVATE PS2AssetWorkbench::core)
    if(PS2ASSET_PNG_AVAILABLE)
        target_link_libraries(ps2asset-texture PRIVATE PS2AssetWorkbench::png)
        target_compile_definitions(ps2asset-texture PRIVATE PS2ASSET_HAS_PNG=1)
    endif()
endif()

if(PS2ASSET_BUILD_TOOLS)
    add_executable(ps2asset-texture-validate tools/validate_textures.cpp)
    target_link_libraries(ps2asset-texture-validate PRIVATE PS2AssetWorkbench::core)
    if(PS2ASSET_PNG_AVAILABLE)
        target_link_libraries(ps2asset-texture-validate PRIVATE PS2AssetWorkbench::png)
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
        add_executable(ps2asset-workbench
            src/gui/main.cpp
            src/gui/main_window.cpp
            src/gui/main_window.hpp
            src/gui/plugin_manager.cpp
            src/gui/plugin_manager.hpp
            include/ps2asset/workbench/plugin_interface.hpp
        )
        target_link_libraries(ps2asset-workbench PRIVATE
            PS2AssetWorkbench::core
            ${PS2ASSET_QT_PACKAGE}::Widgets
        )
        target_compile_features(ps2asset-workbench PRIVATE cxx_std_17)
        set_target_properties(ps2asset-workbench PROPERTIES CXX_EXTENSIONS OFF)
        if(MSVC)
            target_compile_options(ps2asset-workbench PRIVATE /W4 /permissive-)
        else()
            target_compile_options(ps2asset-workbench PRIVATE -Wall -Wextra -Wpedantic)
        endif()

        if(PS2ASSET_BUILD_SAMPLE_PLUGIN)
            add_library(ps2asset_sample_plugin MODULE
                src/gui/plugins/sample_plugin.cpp
                include/ps2asset/workbench/plugin_interface.hpp
            )
            target_include_directories(ps2asset_sample_plugin PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
            target_link_libraries(ps2asset_sample_plugin PRIVATE ${PS2ASSET_QT_PACKAGE}::Widgets)
            target_compile_features(ps2asset_sample_plugin PRIVATE cxx_std_17)
            set_target_properties(ps2asset_sample_plugin PROPERTIES
                CXX_EXTENSIONS OFF
                OUTPUT_NAME "ps2asset_sample_plugin"
                LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins"
            )
            target_compile_definitions(ps2asset-workbench PRIVATE PS2ASSET_EXPECT_SAMPLE_PLUGIN=1)
            install(TARGETS ps2asset_sample_plugin
                LIBRARY DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins"
                RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/plugins"
            )
        endif()

        message(STATUS "Desktop GUI: enabled (${PS2ASSET_QT_PACKAGE} Widgets)")
    else()
        message(WARNING "Qt Widgets development files were not found; the GUI target is skipped. Install Qt 6 or Qt 5 Widgets development packages and reconfigure to build ps2asset-workbench.")
    endif()
endif()

if(BUILD_TESTING)
    add_executable(ps2asset-inspect-tests tests/test_inspect.cpp)
    target_link_libraries(ps2asset-inspect-tests PRIVATE PS2AssetWorkbench::core)
    add_test(NAME header-inspection COMMAND ps2asset-inspect-tests)

    add_executable(ps2asset-tim2-tests tests/test_tim2.cpp)
    target_link_libraries(ps2asset-tim2-tests PRIVATE PS2AssetWorkbench::core)
    add_test(NAME tim2-decode-and-replace COMMAND ps2asset-tim2-tests)

    if(PS2ASSET_ENABLE_ARCHIVE AND LibArchive_FOUND)
        add_executable(ps2asset-archive-tests tests/test_archive.cpp)
        if(TARGET LibArchive::LibArchive)
            target_link_libraries(ps2asset-archive-tests PRIVATE PS2AssetWorkbench::core LibArchive::LibArchive)
        else()
            target_include_directories(ps2asset-archive-tests PRIVATE ${LibArchive_INCLUDE_DIRS})
            target_link_libraries(ps2asset-archive-tests PRIVATE PS2AssetWorkbench::core ${LibArchive_LIBRARIES})
        endif()
        add_test(NAME zip-archive-listing COMMAND ps2asset-archive-tests)
    endif()
    if(TARGET ps2asset-texture-validate)
        add_test(NAME texture-replacement-roundtrip-self-test COMMAND ps2asset-texture-validate --self-test)
    endif()
    if(PS2ASSET_PNG_AVAILABLE)
        add_executable(ps2asset-png-tests tests/test_png.cpp)
        target_link_libraries(ps2asset-png-tests PRIVATE PS2AssetWorkbench::png)
        add_test(NAME png-rgba-roundtrip COMMAND ps2asset-png-tests)
    endif()
endif()

install(TARGETS ps2asset_core
    EXPORT PS2AssetWorkbenchTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(TARGET ps2asset-inspect)
    install(TARGETS ps2asset-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-texture)
    install(TARGETS ps2asset-texture RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset-texture-validate)
    install(TARGETS ps2asset-texture-validate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(TARGET ps2asset_png)
    install(TARGETS ps2asset_png EXPORT PS2AssetWorkbenchTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
if(TARGET ps2asset-workbench)
    install(TARGETS ps2asset-workbench RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
