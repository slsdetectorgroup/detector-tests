cmake_minimum_required(VERSION 3.12)
project(myintegration)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(slsDetectorPackage 6.1 REQUIRED)
find_package(fmt REQUIRED)
add_executable(start-stop start-stop.cpp)
target_link_libraries(start-stop slsDetectorShared fmt::fmt)
