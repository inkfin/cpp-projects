cmake_minimum_required(VERSION 3.29)

set(CMAKE_C_STANDARD 23)

#===== Project Build Options ===========

project(LuaVM LANGUAGES C VERSION 0.1)

set(BINARY_NAME ${PROJECT_NAME})

if(PROJECT_BINARY_DIR STREQUAL PROJECT_SOURCE_DIR)
    message(SEND_ERROR "The binary directory of CMake cannot be the same as source directory!")
endif()

if(WIN32)
    # disable min/max macros to avoid conflicts with std::min/max
    add_definitions(-DNOMINMAX -D_USE_MATH_DEFINES)
endif()

#===== Dependencies ====================

add_subdirectory(dependencies/lua-5.3.6)

#===== Source Files ====================

file(GLOB_RECURSE LUAVM_SOURCE_FILES CONFIGURE_DEPENDS
    source/luavm/*.c
)

include_directories(
    source/luavm
)

add_executable(${BINARY_NAME}
    ${LUASRC_SOURCE_FILES}
    ${LUAVM_SOURCE_FILES}
)


#===== Target Build Options ============

target_compile_options(${BINARY_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall>
)

#===== Link Libraries ==================

target_link_libraries(${BINARY_NAME}
    PRIVATE
        lua::lua
)


#===== Output Directories ==============

set_target_properties(${BINARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    ## Visual Studio specific
    # VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${BINARY_NAME}>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${BINARY_NAME}>"
)
message(STATUS "Set ${BINARY_NAME} binary output directory to ${CMAKE_SOURCE_DIR}/target")


#===== Copy Commands ===================

if(WIN32)
    file(GLOB LUA_FILES_TO_COPY
        "${CMAKE_SOURCE_DIR}/*.lua"
    )
    add_custom_command(
        TARGET ${BINARY_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -E copy_if_different
                ${LUA_FILES_TO_COPY}
                $<TARGET_FILE_DIR:${BINARY_NAME}> # target
        COMMAND_EXPAND_LISTS
        COMMENT "Copying lua files to target directory..."
    )
endif()

