cmake_minimum_required(VERSION 3.29)

#===== Project Build Options ===========

project(LearnWGPU LANGUAGES CXX VERSION 0.1)

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(SEND_ERROR "The binary directory of CMake cannot be the same as source directory!")
endif()

if(WIN32)
    # disable min/max macros to avoid conflicts with std::min/max
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

option(USE_CCACHE "Use ccache for compilation" ON)
if(USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "Found CCache: ${CCACHE_PROGRAM}")
        if(NOT MSVC)
            set_property(GLOBAL PROPERTY C_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
            set_property(GLOBAL PROPERTY CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
        else()
            # Masquerade cl.exe with ccache
            #   ref: <https://github.com/ccache/ccache/wiki/MS-Visual-Studio#usage-with-cmake>
            #file(COPY_FILE ${CCACHE_PROGRAM} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)

            # By default Visual Studio generators will use /Zi which is not compatible
            # with ccache, so tell Visual Studio to use /Z7 instead.
            # message(STATUS "Setting MSVC debug information format to 'Embedded'")
            # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

            # set(CMAKE_VS_GLOBALS
            #     "CLToolExe=cl.exe"
            #     "CLToolPath=${CMAKE_BINARY_DIR}"
            #     "UseMultiToolTask=true"
            #     "DebugInformationFormat=OldStyle")
        endif()
    endif()
endif()

# set(CMAKE_FIND_DEBUG_MODE 1)

#===== Dependencies ====================

# Dawn
set (DAWN_PLATFORM_DIR "${CMAKE_SOURCE_DIR}/dawn/${CMAKE_SYSTEM_NAME}")

if(NOT EXISTS ${DAWN_PLATFORM_DIR})
    message(FATAL_ERROR "DAWN_PLATFORM_DIR is not set or does not exist: ${DAWN_PLATFORM_DIR}")
endif()

list(APPEND CMAKE_PREFIX_PATH "${DAWN_PLATFORM_DIR}/lib/cmake/Dawn")

# include(cmake/helpers.cmake)

find_package(Dawn REQUIRED CONFIG)

#===== Projects ========================

add_subdirectory(src/triangle)

#=======================================
#===== CMake Cheat Sheet ===============
#
# CMAKE_BINARY_DIR: the directory where the compiled files are stored
# CMAKE_SOURCE_DIR: the directory where the top-level CMakeLists.txt file is located
# CMAKE_CURRENT_BINARY_DIR: the directory where the currently processed CMakeLists.txt file is compiled
# CMAKE_CURRENT_SOURCE_DIR: the directory where the currently processed CMakeLists.txt file is located
# CMAKE_MODULE_PATH: a list of directories containing additional CMake modules
# CMAKE_CURRENT_LIST_FILE: the full path to the CMakeLists.txt file currently being processed
# CMAKE_CURRENT_LIST_DIR: the directory of the CMakeLists.txt file currently being processed
#
# PROJECT_NAME: the name of the project defined by the project() command
# PROJECT_SOURCE_DIR: the directory where the top-level CMakeLists.txt file is located
# PROJECT_BINARY_DIR: the directory where the compiled files are stored
#
# CMAKE_INCLUDE_PATH: the list of directories that are searched for include files
# CMAKE_LIBRARY_PATH: the list of directories that are searched for libraries
#
# CMAKE_SYSTEM: true if CMake is built for the system
# CMAKE_SYSTEM_NAME: the name of the system CMake is running on
# CMAKE_SYSTEM_VERSION: the version of the system CMake is running on
# CMAKE_SYSTEM_PROCESSOR: the processor CMake is running on
#
# UNIX: true if system is UNIX-like
# WIN32: true if system is Windows
# APPLE: true if system is Apple
# MSVC: true if the compiler is MSVC
# MSVC_VERSION: the version of MSVC
# CMAKE_C_COMPILER: the compiler used for C
# CMAKE_CXX_COMPILER: the compiler used for C++
#
# ======================================
