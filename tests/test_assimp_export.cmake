if(NOT DEFINED TOOL OR NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "TOOL, INPUT and OUTPUT are required")
endif()
file(REMOVE "${OUTPUT}")
execute_process(
    COMMAND "${TOOL}" "${INPUT}" "${OUTPUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE stdout
    ERROR_VARIABLE stderr
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "GLB exporter failed (${result})\nstdout=${stdout}\nstderr=${stderr}")
endif()
if(NOT EXISTS "${OUTPUT}")
    message(FATAL_ERROR "GLB output was not created")
endif()
file(READ "${OUTPUT}" magic OFFSET 0 LIMIT 4 HEX)
if(NOT "${magic}" STREQUAL "676c5446")
    message(FATAL_ERROR "Output does not start with glTF binary magic; got ${magic}")
endif()
file(READ "${OUTPUT}" version OFFSET 4 LIMIT 4 HEX)
if(NOT "${version}" STREQUAL "02000000")
    message(FATAL_ERROR "Output is not GLB v2; version bytes ${version}")
endif()
file(REMOVE "${OUTPUT}")
message(STATUS "Assimp GLB export fixture passed")
