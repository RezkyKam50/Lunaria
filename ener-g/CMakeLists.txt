cmake_minimum_required(VERSION 3.16)
project(EnergyMonitor)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

qt_standard_project_setup()

qt_add_executable(EnergyMonitor
    main.cpp
    energymonitor.cpp
    energymonitor.h
)

# Use keyword signature for target_link_libraries
target_link_libraries(EnergyMonitor 
    PRIVATE 
        Qt6::Core 
        Qt6::Widgets 
        Qt6::Charts
)