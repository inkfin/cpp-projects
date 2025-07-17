project(TriangleApp
    VERSION 0.1.0
    LANGUAGES CXX)

set(WGPU_BACKEND "DAWN" CACHE STRING "WGPU backend to use. Options: DAWN, WGPU_NATIVE, EMSCRIPTEN")
set_property(CACHE WGPU_BACKEND PROPERTY STRINGS DAWN WGPU_NATIVE EMSCRIPTEN)

#===== Source Files ====================

file(GLOB TriangleApp_SOURCES
    main.cpp
)

# include_directories(.)

add_executable(TriangleApp ${TriangleApp_SOURCES})

set_target_properties(TriangleApp PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

#===== Target Build Options ============

target_compile_options(TriangleApp
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

#===== Link Libraries ==================

target_link_libraries(TriangleApp PUBLIC dawn::webgpu_dawn)

# add WGPU_BACKEND_XXX macro to the target
set_target_properties(TriangleApp PROPERTIES COMPILE_DEFINITIONS "WGPU_BACKEND_${WGPU_BACKEND}")
message(STATUS "Setting WGPU_BACKEND_${WGPU_BACKEND} compile definition")

if(WGPU_BACKEND STREQUAL "EMSCRIPTEN")
    set_target_properties(TriangleApp PROPERTIES SUFFIX ".html")
    message(STATUS "Building for Emscripten backend")
endif()

#===== Output Directories ==============

set_target_properties(TriangleApp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    ## Visual Studio specific
    # VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:TriangleApp>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:TriangleApp>"
    # VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%"
)
message(STATUS "Set TriangleApp binary output directory to ${CMAKE_SOURCE_DIR}/target")


#===== Copy Commands ===================

# cmake build time copy
# file(GLOB EXTRA_DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/lib/*.dll")
# add_custom_command(
#     TARGET TriangleApp
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND}
#             -E copy_if_different
#             ${EXTRA_DLLS_TO_COPY}
#             $<TARGET_RUNTIME_DLLS:TriangleApp>
#             $<TARGET_FILE_DIR:TriangleApp> # target
#     COMMAND_EXPAND_LISTS
#     COMMENT "Copying dlls to target directory..."
# )


