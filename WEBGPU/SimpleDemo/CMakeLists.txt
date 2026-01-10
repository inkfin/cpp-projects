cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)


#===== Project Build Options ===========

project(WGPUSimpleDemo LANGUAGES CXX VERSION 0.1)

set(BINARY_NAME app)

#===== Dependencies ====================

include(../cmake/webgpu_glfw_backend.cmake)

#===== Targets =========================

file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    *.h *.cpp
)

add_executable(${BINARY_NAME} ${SOURCE_FILES})


#===== Target Build Options ============

target_compile_options(${BINARY_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

target_link_libraries(${BINARY_NAME}
    PRIVATE
        webgpu_glfw_backend
)


#===== Copy Commands ===================

# cmake build time copy
add_custom_command(
    TARGET ${BINARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy_if_different
            $<TARGET_FILE_DIR:${BINARY_NAME}>/${BINARY_NAME}.js
            $<TARGET_FILE_DIR:${BINARY_NAME}>/${BINARY_NAME}.wasm
            ${CMAKE_SOURCE_DIR}/target/
    COMMAND_EXPAND_LISTS
    COMMENT "Copying dlls to target directory..."
)


