cmake_minimum_required(VERSION 3.29)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

#===== Project Build Options ===========

project(TensorEdit LANGUAGES CXX VERSION 0.1)

set(BINARY_NAME ${PROJECT_NAME})

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(SEND_ERROR "The binary directory of CMake cannot be the same as source directory!")
endif()

if(WIN32)
    # disable min/max macros to avoid conflicts with std::min/max
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

#===== Dependencies ====================

# Lua added in parent CMakeLists.txt

#===== Source Files ====================

include_directories(
    "${CMAKE_SOURCE_DIR}/source"
)

file(GLOB LUA_TENSOR_SOURCES CONFIGURE_DEPENDS
    *.cpp
)

add_executable(${BINARY_NAME}
    ${LUA_TENSOR_SOURCES}
)


#===== Target Build Options ============

target_compile_options(${BINARY_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)

#===== Link Libraries ==================

target_link_libraries(${BINARY_NAME}
    PRIVATE
        lua::lua
)


#===== Output Directories ==============

set_target_properties(${BINARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/${PROJECT_NAME}/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/${PROJECT_NAME}/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/${PROJECT_NAME}/$<CONFIG>"
    ## Visual Studio specific
    # VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${BINARY_NAME}>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${BINARY_NAME}>"
)
message(STATUS "Set ${BINARY_NAME} binary output directory to target/${PROJECT_NAME}/")


#===== Copy Commands ===================

set(LUA_SCRIPTS_DIR
    "${CMAKE_SOURCE_DIR}/scripts"
)
add_custom_command(
    TARGET ${BINARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy_directory_if_different
            ${LUA_SCRIPTS_DIR}
            $<TARGET_FILE_DIR:${BINARY_NAME}>/scripts # target
    COMMAND_EXPAND_LISTS
    COMMENT "Copying lua files to target directory..."
)


