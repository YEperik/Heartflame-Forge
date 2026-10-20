cmake_minimum_required(VERSION 3.16)
project(ps2_asset_inspector VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(ps2asset_inspect src/inspect.cpp)
target_include_directories(ps2asset_inspect PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(ps2asset_inspect PUBLIC cxx_std_17)

if(MSVC)
  target_compile_options(ps2asset_inspect PRIVATE /W4 /permissive-)
else()
  target_compile_options(ps2asset_inspect PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()

add_executable(ps2asset-inspect src/main.cpp)
target_link_libraries(ps2asset-inspect PRIVATE ps2asset_inspect)

if(BUILD_TESTING)
  add_executable(ps2asset-inspect-tests tests/test_inspect.cpp)
  target_link_libraries(ps2asset-inspect-tests PRIVATE ps2asset_inspect)
  add_test(NAME header-inspection COMMAND ps2asset-inspect-tests)
endif()
