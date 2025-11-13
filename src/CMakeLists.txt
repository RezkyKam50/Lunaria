cmake_minimum_required(VERSION 3.16)
project(Lunaria)

# TODO: QtCharts is planned for performance graph eg. (GPU utils, CPU utils, etc.)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia Charts)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# LLAMA.CPP path
set(LLAMA_CPP_DIR "../llama.cpp")
set(LLAMA_BUILD_DIR "${LLAMA_CPP_DIR}/build_llama")

include_directories(
    ${LLAMA_CPP_DIR}/include
    ${LLAMA_CPP_DIR}/common
    ${LLAMA_CPP_DIR}/ggml/include
)

# WHISPER.CPP path
set(WHISPER_CPP_DIR "../whisper.cpp")
set(WHISPER_BUILD_DIR "${WHISPER_CPP_DIR}/build_whisper")

include_directories(
    ${WHISPER_CPP_DIR}/include
    ${WHISPER_CPP_DIR}/ggml/include
)

# Find Poppler (for PDF support)
find_package(PkgConfig REQUIRED)
pkg_check_modules(POPPLER REQUIRED poppler-cpp)

if(POPPLER_FOUND)
    message(STATUS "=== POPPLER Configuration ===")
    message(STATUS "Poppler version: ${POPPLER_VERSION}")
    message(STATUS "Poppler include dirs: ${POPPLER_INCLUDE_DIRS}")
    message(STATUS "Poppler libraries: ${POPPLER_LIBRARIES}")
    message(STATUS "Poppler library dirs: ${POPPLER_LIBRARY_DIRS}")
    
    include_directories(${POPPLER_INCLUDE_DIRS})
    link_directories(${POPPLER_LIBRARY_DIRS})
else()
    message(FATAL_ERROR "Poppler not found. Please install poppler development files (e.g., libpoppler-cpp-dev on Debian/Ubuntu)")
endif()

#* enable BUILD_SHARED_LIBS 

# Find llama.cpp libraries
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

# Find whisper.cpp libraries
find_library(WHISPER_LIB 
    NAMES whisper libwhisper
    PATHS 
        ${WHISPER_BUILD_DIR}/bin
        ${WHISPER_BUILD_DIR}/src
        ${WHISPER_BUILD_DIR}
    NO_DEFAULT_PATH
)

find_library(WHISPER_GGML_LIB 
    NAMES ggml libggml
    PATHS 
        ${WHISPER_BUILD_DIR}/bin
        ${WHISPER_BUILD_DIR}/ggml/src
        ${WHISPER_BUILD_DIR}
    NO_DEFAULT_PATH
)

# Check 1: llama.cpp
message(STATUS "=== LLAMA.CPP Configuration ===")
message(STATUS "LLAMA_CPP_DIR: ${LLAMA_CPP_DIR}")
message(STATUS "LLAMA_BUILD_DIR: ${LLAMA_BUILD_DIR}")
message(STATUS "LLAMA_LIB: ${LLAMA_LIB}")
message(STATUS "GGML_LIB: ${GGML_LIB}")
message(STATUS "COMMON_LIB: ${COMMON_LIB}")

# Check 2: whisper.cpp
message(STATUS "=== WHISPER.CPP Configuration ===")
message(STATUS "WHISPER_CPP_DIR: ${WHISPER_CPP_DIR}")
message(STATUS "WHISPER_BUILD_DIR: ${WHISPER_BUILD_DIR}")
message(STATUS "WHISPER_LIB: ${WHISPER_LIB}")
message(STATUS "WHISPER_GGML_LIB: ${WHISPER_GGML_LIB}")

# Verify llama.cpp libraries
if(NOT LLAMA_LIB)
    message(FATAL_ERROR "Could not find llama library. Please build llama.cpp first.")
endif()

if(NOT GGML_LIB)
    message(FATAL_ERROR "Could not find ggml library. Please build llama.cpp first.")
endif()

if(NOT COMMON_LIB)
    message(WARNING "Could not find common library. Will try to link without it.")
endif()

# Verify whisper.cpp libraries
if(NOT WHISPER_LIB)
    message(FATAL_ERROR "Could not find whisper library. Please build whisper.cpp first with: cmake -B build_whisper -DBUILD_SHARED_LIBS=ON && cmake --build build_whisper")
endif()

if(NOT WHISPER_GGML_LIB)
    message(WARNING "Could not find whisper's ggml library. Will try to link without it.")
endif()

add_executable(Lunaria 
    lunaria.cpp 
    llamaworker.cpp 
    llamaworker.h
    whisperworker.cpp
    whisperworker.h
    settingsdialog.cpp
    settingsdialog.h
)

set(LINK_LIBS
    Qt6::Core 
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::Charts
    ${LLAMA_LIB}
    ${GGML_LIB}
    ${WHISPER_LIB}
    ${POPPLER_LIBRARIES}
)

if(COMMON_LIB)
    list(APPEND LINK_LIBS ${COMMON_LIB})
endif()

if(WHISPER_GGML_LIB)
    list(APPEND LINK_LIBS ${WHISPER_GGML_LIB})
endif()

target_link_libraries(Lunaria ${LINK_LIBS})

target_compile_definitions(Lunaria PRIVATE 
    "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>"
)