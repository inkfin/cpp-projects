cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)


#===== Project Build Options ===========

project(CoolAnims LANGUAGES CXX VERSION 0.1)

set(BINARY_NAME app)

if(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE Release)
message(STATUS "Setting build type to 'Release' as none was specified.")
endif()

#===== Dependencies ====================

# include(../cmake/helpers.cmake)

#===== Source Files ====================

# aux_source_directory(src SOURCE_FILES)
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS
    src/*.h
    src/*.cpp
)

include_directories(include)

set(CMAKE_EXECUTABLE_SUFFIX ".js")
add_executable(${BINARY_NAME} ${SOURCE_FILES})


#===== Target Build Options ============

target_compile_options(${BINARY_NAME}
    PRIVATE
        "--use-port=emdawnwebgpu"
)

target_link_options(${BINARY_NAME}
    PRIVATE
        "--use-port=emdawnwebgpu"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sMODULARIZE=1"
        "-sEXPORT_ES6=1"
        "-sENVIRONMENT=web"
)

# target_link_libraries(${BINARY_NAME} PUBLIC Eigen3::Eigen)


#===== Output Directories ==============

file(GLOB WASM_FILES "${CMAKE_BINARY_DIR}/*.wasm")
add_custom_command(
    TARGET ${BINARY_NAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
            -E copy_if_different
            ${WASM_FILES}
    COMMAND_EXPAND_LISTS
    COMMENT "Copying wasm to target directory..."
)
