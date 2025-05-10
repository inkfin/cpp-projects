cmake_minimum_required(VERSION 3.29)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)

#===== Project Build Options ===========

project(AutoExecEditor LANGUAGES CXX VERSION 0.1)

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
            file(COPY_FILE ${CCACHE_PROGRAM} ${CMAKE_BINARY_DIR}/cl.exe ONLY_IF_DIFFERENT)

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

#===== Configurations ==================

string(TIMESTAMP BUILD_DATE "%Y-%m-%d %H:%M" UTC)

set(BUILD_INFO_IN   "${CMAKE_SOURCE_DIR}/cmake/build_info.h.in")
set(BUILD_INFO_OUT  "${CMAKE_BINARY_DIR}/generated/build_info.h")

configure_file(
  ${BUILD_INFO_IN}
  ${BUILD_INFO_OUT}
  @ONLY
)

include_directories(${CMAKE_BINARY_DIR}/generated)

#===== Dependencies ====================

find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

add_subdirectory(imgui)

#===== Source Files ====================

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    include/*.h
    src/*.h
    src/*.cpp
)

include_directories(include)

add_executable(${BINARY_NAME} ${SOURCE_FILES})


#===== Target Build Options ============

target_compile_options(${BINARY_NAME} PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

if(MSVC)
    # supress warnings
    target_link_options(${BINARY_NAME} PRIVATE "/NODEFAULTLIB:LIBCMT")
endif()

#===== Link Libraries ==================

target_link_libraries(${BINARY_NAME} PUBLIC
    imgui
    glfw
    GLEW::GLEW 
)


#===== Output Directories ==============

set_target_properties(${BINARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    ## Visual Studio specific
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${BINARY_NAME}>"
    # VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${BINARY_NAME}>"
)
message(STATUS "Set ${BINARY_NAME} binary output directory to ${CMAKE_SOURCE_DIR}/target")


#===== Copy Commands ===================

# local assets
add_custom_command(
    TARGET ${BINARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy_directory_if_different
            ${CMAKE_SOURCE_DIR}/assets
            $<TARGET_FILE_DIR:${BINARY_NAME}>/assets
    COMMAND_EXPAND_LISTS
    COMMENT "Copying assets to target directory..."
)

# cmake build time copy
# file(GLOB EXTRA_DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/lib/*.dll")
# add_custom_command(
#     TARGET ${BINARY_NAME}
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND}
#             -E copy
#             -t $<TARGET_FILE_DIR:${BINARY_NAME}>
#             ${EXTRA_DLLS_TO_COPY}
#             $<TARGET_RUNTIME_DLLS:${BINARY_NAME}>
#     COMMAND_EXPAND_LISTS
#     COMMENT "Copying dlls to target directory..."
# )
