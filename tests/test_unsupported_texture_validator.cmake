if(NOT DEFINED TOOL OR NOT DEFINED INPUT)
  message(FATAL_ERROR "TOOL and INPUT are required")
endif()
execute_process(
  COMMAND "${TOOL}" "${INPUT}"
  RESULT_VARIABLE result
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT result EQUAL 3)
  message(FATAL_ERROR "Expected exit code 3 for a file with no supported pictures; got ${result}\nstdout=${out}\nstderr=${err}")
endif()
if(NOT out MATCHES "PARTIAL .*unsupported picture profile")
  message(FATAL_ERROR "Expected an explicit PARTIAL result for the unsupported profile\nstdout=${out}")
endif()
if(NOT out MATCHES "Unsupported profile pictures: 1" OR NOT out MATCHES "round-trips: 0")
  message(FATAL_ERROR "Unsupported picture was not counted correctly\nstdout=${out}")
endif()
message(STATUS "Unsupported TIM2 profile is reported explicitly as PARTIAL and not falsely marked PASS")
