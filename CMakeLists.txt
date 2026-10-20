cmake_minimum_required(VERSION 3.16)
project(PS2AssetWorkbench VERSION 0.2.0 LANGUAGES CXX)

include(CTest)
include(GNUInstallDirs)

option(PS2ASSET_BUILD_GUI "Build the Qt Widgets desktop application when Qt is available" ON)
option(PS2ASSET_BUILD_CLI "Build command-line inspection and texture tools" ON)
option(PS2ASSET_ENABLE_ARCHIVE "Enable browsing TIM2 textures inside ZIP files when libarchive is available" ON)

# Core library: format parsers are kept independent from the GUI toolkit.
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

if(PS2ASSET_BUILD_CLI)
    add_executable(ps2asset-inspect src/main.cpp)
    target_link_libraries(ps2asset-inspect PRIVATE PS2AssetWorkbench::core)

    add_executable(ps2asset-texture src/texture_cli.cpp)
    target_link_libraries(ps2asset-texture PRIVATE PS2AssetWorkbench::core)
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
        add_executable(ps2asset-workbench
            src/gui/main.cpp
            src/gui/main_window.cpp
            src/gui/main_window.hpp
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
if(TARGET ps2asset-workbench)
    install(TARGETS ps2asset-workbench RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
