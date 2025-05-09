cmake_minimum_required(VERSION 3.29)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

#===== VCPKG ===========================

# if(NOT CMAKE_TOOLCHAIN_FILE)
#     set(CMAKE_TOOLCHAIN_FILE "vcpkg/scripts/buildsystems/vcpkg.cmake"
#         CACHE FILEPATH "Path to vcpkg toolchain file" FORCE)
# endif()


#===== Project Build Options ===========

project(HelloWorld LANGUAGES CXX VERSION 0.1)

set(BINARY_NAME ${PROJECT_NAME})

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(SEND_ERROR "The binary directory of CMake cannot be the same as source directory!")
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
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
            message(STATUS "Setting MSVC debug information format to 'Embedded'")
            set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")

            set(CMAKE_VS_GLOBALS
                "CLToolExe=cl.exe"
                "CLToolPath=${CMAKE_BINARY_DIR}"
                "UseMultiToolTask=true"
                "DebugInformationFormat=OldStyle")
        endif()
    endif()
endif()

# message(STATUS "CMAKE_MODULE_PATH: ${CMAKE_MODULE_PATH}")


#===== Dependencies ====================

# include(cmake/helpers.cmake)
# add_subdirectory(third_party/googletest)

# find_package(Eigen3 COMPONENTS Eigen REQUIRED)

#===== Source Files ====================

# aux_source_directory(src SOURCE_FILES)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    src/*.cpp src/*.h include/*.h
)

include_directories(include)

add_executable(${BINARY_NAME} ${SOURCE_FILES})


#===== Target Build Options ============

target_compile_options(${BINARY_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

# option(WITH_TBB "set to ON to enable TBB, OFF to disable TBB." ON)
# if (WITH_TBB)
#     target_compile_definitions(${BINARY_NAME} PUBLIC WITH_TBB)
#     find_package(TBB REQUIRED)
#     target_link_libraries(${BINARY_NAME} PUBLIC TBB:tbb)
# endif()


#===== Link Libraries ==================

# target_link_libraries(${BINARY_NAME} PUBLIC Eigen3::Eigen)


#===== Output Directories ==============

set_target_properties(${BINARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    ## Visual Studio specific
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${BINARY_NAME}>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${BINARY_NAME}>"
    VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%"
)
message(STATUS "Set ${BINARY_NAME} binary output directory to ${CMAKE_SOURCE_DIR}/target")


#===== Copy Commands ===================

# cmake configure time copy
# if(EXISTS "${CMAKE_BINARY_DIR}/compile_commands.json")
#     file(COPY "${CMAKE_BINARY_DIR}/compile_commands.json" DESTINATION ${CMAKE_SOURCE_DIR})
# endif()

# cmake build time copy
file(GLOB EXTRA_DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/lib/*.dll")
add_custom_command(
    TARGET ${BINARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy
            -t $<TARGET_FILE_DIR:${BINARY_NAME}>
            ${EXTRA_DLLS_TO_COPY}
            $<TARGET_RUNTIME_DLLS:${BINARY_NAME}>
    COMMAND_EXPAND_LISTS
    COMMENT "Copying dlls to target directory..."
)


#======================================
#===== CMake Cheat Sheet ==============
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

