cmake_minimum_required(VERSION 3.20)

#===== Project Build Options ===========

project(DawnDemo
    LANGUAGES CXX
    VERSION 0.1
)

if(WIN32)
    # disable min/max macros to avoid conflicts with std::min/max
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

# set(CMAKE_FIND_DEBUG_MODE 1)

#===== Dependencies ====================

# Dawn
set (DAWN_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/dawn/${CMAKE_SYSTEM_NAME}")

if(EXISTS ${DAWN_PLATFORM_DIR})
    message(STATUS "Using Dawn platform directory: ${DAWN_PLATFORM_DIR}")
else()
    message(FATAL_ERROR "Dawn CMake configuration not found in: ${DAWN_PLATFORM_DIR}/lib/cmake/Dawn")
endif()

list(APPEND CMAKE_PREFIX_PATH "${DAWN_PLATFORM_DIR}/lib/cmake/Dawn")

# include(cmake/helpers.cmake)

find_package(Dawn REQUIRED CONFIG)

#===== Projects ========================

add_subdirectory(src/triangle)

