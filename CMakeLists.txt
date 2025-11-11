cmake_minimum_required(VERSION 3.16)
project(Lunaria)

# Find required Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Path to llama.cpp
set(LLAMA_CPP_DIR "llama.cpp")
set(LLAMA_BUILD_DIR "${LLAMA_CPP_DIR}/build_llama")

# Include directories
include_directories(
    ${LLAMA_CPP_DIR}/include
    ${LLAMA_CPP_DIR}/common
    ${LLAMA_CPP_DIR}/ggml/include
)


# Find the shared libraries in the bin directory
find_library(LLAMA_LIB 
    NAMES llama libllama
    PATHS 
        ${LLAMA_BUILD_DIR}/bin
        ${LLAMA_BUILD_DIR}
    NO_DEFAULT_PATH
)

find_library(GGML_LIB 
    NAMES ggml libggml
    PATHS 
        ${LLAMA_BUILD_DIR}/bin
        ${LLAMA_BUILD_DIR}
    NO_DEFAULT_PATH
)

find_library(COMMON_LIB 
    NAMES common libcommon
    PATHS 
        ${LLAMA_BUILD_DIR}/common
        ${LLAMA_BUILD_DIR}/bin
        ${LLAMA_BUILD_DIR}
    NO_DEFAULT_PATH
)

# Debug: Print library paths
message(STATUS "LLAMA_CPP_DIR: ${LLAMA_CPP_DIR}")
message(STATUS "LLAMA_BUILD_DIR: ${LLAMA_BUILD_DIR}")
message(STATUS "LLAMA_LIB: ${LLAMA_LIB}")
message(STATUS "GGML_LIB: ${GGML_LIB}")
message(STATUS "COMMON_LIB: ${COMMON_LIB}")

# Check if libraries were found
if(NOT LLAMA_LIB)
    message(FATAL_ERROR "Could not find llama library. Please build llama.cpp first.")
endif()

if(NOT GGML_LIB)
    message(FATAL_ERROR "Could not find ggml library. Please build llama.cpp first.")
endif()

# Common library is optional in some builds
if(NOT COMMON_LIB)
    message(WARNING "Could not find common library. Will try to link without it.")
endif()

# Add the executable
add_executable(Lunaria lunaria.cpp)



# Link libraries
if(COMMON_LIB)
    target_link_libraries(Lunaria 
        Qt6::Core 
        Qt6::Widgets
        ${LLAMA_LIB}
        ${COMMON_LIB}
        ${GGML_LIB}
    )
else()
    target_link_libraries(Lunaria 
        Qt6::Core 
        Qt6::Widgets
        ${LLAMA_LIB}
        ${GGML_LIB}
    )
endif()

# Set properties for the target
target_compile_definitions(Lunaria PRIVATE 
    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>"
)