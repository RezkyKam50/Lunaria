cmake_minimum_required(VERSION 3.16)
project(Lunaria)

# Find required Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# Add the executable
add_executable(Lunaria lunaria.cpp)

# Link Qt6 libraries
target_link_libraries(Lunaria Qt6::Core Qt6::Widgets)

# Set properties for the target
target_compile_definitions(Lunaria PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>")