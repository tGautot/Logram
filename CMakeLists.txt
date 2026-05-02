# ================= CONFIGURING ===================================
cmake_minimum_required(VERSION 3.21)

project(Logram VERSION 0.0)

set(CMAKE_CXX_FLAGS_WARNINGS -Wall -Werror)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_DIR ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_BINARY_DIR ${CMAKE_BUILD_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BUILD_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BUILD_DIR}/lib)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GCC_INSTALL_PREFIX /usr/bin)

# If you wish to debug Logram
# add_compile_options(-g)
add_compile_options(-O3)
# If you wish to do some performance benchmarking
# add_definitions(-DPERF_ANALYZE_ON)

# ================= BUILDING ======================================

add_subdirectory(src)
add_subdirectory(tests)


