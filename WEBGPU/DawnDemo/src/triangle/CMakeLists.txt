project(TriangleApp
    VERSION 0.1.0
    LANGUAGES CXX)

set(WEBGPU_BACKEND "DAWN" CACHE STRING "WGPU backend to use. Options: DAWN, WGPU_NATIVE, EMSCRIPTEN")
set_property(CACHE WEBGPU_BACKEND PROPERTY STRINGS DAWN WGPU_NATIVE EMSCRIPTEN)

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

# add WEBGPU_BACKEND_XXX macro to the target
set_target_properties(TriangleApp PROPERTIES COMPILE_DEFINITIONS "WEBGPU_BACKEND_${WEBGPU_BACKEND}")
message(STATUS "Setting WEBGPU_BACKEND_${WEBGPU_BACKEND} compile definition")

if(WEBGPU_BACKEND STREQUAL "EMSCRIPTEN")
    set_target_properties(TriangleApp PROPERTIES SUFFIX ".html")
    message(STATUS "Building for Emscripten backend")
endif()

#===== Copy Commands ===================

# cmake build time copy
file(GLOB EXPORT_LIBS "${DAWN_PLATFORM_DIR}/bin/*.dll")
add_custom_command(
    TARGET TriangleApp
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy_if_different
            ${EXPORT_LIBS}
            $<TARGET_RUNTIME_DLLS:TriangleApp>
            $<TARGET_FILE_DIR:TriangleApp> # target
    COMMAND_EXPAND_LISTS
    COMMENT "Copying dlls to target directory..."
)

