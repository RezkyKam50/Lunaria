# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/Lunaria_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/Lunaria_autogen.dir/ParseCache.txt"
  "Lunaria_autogen"
  )
endif()
