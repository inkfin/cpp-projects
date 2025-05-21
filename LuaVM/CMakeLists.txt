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

# include(cmake/helpers.cmake)
# add_subdirectory(source/luasrc)

# find_package(Eigen3 COMPONENTS Eigen REQUIRED)

#===== Source Files ====================

# file(GLOB_RECURSE LUASRC_SOURCE_FILES CONFIGURE_DEPENDS
#     source/luasrc/*.c
# )

file(GLOB_RECURSE LUAVM_SOURCE_FILES CONFIGURE_DEPENDS
    source/luavm/*.c
)

include_directories(
    # source/luasrc
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

add_library(Lua54 SHARED IMPORTED)
set_target_properties(Lua54 PROPERTIES
    LINKER_LANGUAGE C
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/lib/lua54.dll"
    IMPORTED_IMPLIB   "${CMAKE_SOURCE_DIR}/lib/lua54.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/source/luadyn"
)

target_link_libraries(${BINARY_NAME}
    PRIVATE
        Lua54
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
    file(GLOB EXTRA_DLLS_TO_COPY
        "${CMAKE_SOURCE_DIR}/lib/lua54.dll"
    )
    add_custom_command(
        TARGET ${BINARY_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND}
                -E copy_if_different
                ${EXTRA_DLLS_TO_COPY}
                $<TARGET_RUNTIME_DLLS:${BINARY_NAME}>
                $<TARGET_FILE_DIR:${BINARY_NAME}> # target
        COMMAND_EXPAND_LISTS
        COMMENT "Copying dlls to target directory..."
    )

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

