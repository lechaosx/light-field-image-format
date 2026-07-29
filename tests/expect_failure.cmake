math(EXPR last_argument "${CMAKE_ARGC} - 1")
foreach(index RANGE 4 ${last_argument})
  list(APPEND command "${CMAKE_ARGV${index}}")
endforeach()

execute_process(COMMAND ${command} RESULT_VARIABLE result)
if(result STREQUAL "0")
  message(FATAL_ERROR "command succeeded")
endif()
