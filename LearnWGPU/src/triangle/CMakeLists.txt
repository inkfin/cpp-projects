project(TriangleApp
    VERSION 0.1.0
    LANGUAGES CXX)

set(BINARY_NAME TriangleApp)

#===== Source Files ====================

file(GLOB SOURCE_FILES
    main.cpp
)

# include_directories(.)

add_executable(${BINARY_NAME} ${SOURCE_FILES})

set_target_properties(${BINARY_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    COMPILE_WARNING_AS_ERROR ON
)

#===== Target Build Options ============

target_compile_options(${BINARY_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/W4>
        $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

#===== Link Libraries ==================

target_link_libraries(${BINARY_NAME} PUBLIC dawn::webgpu_dawn)

#===== Output Directories ==============

set_target_properties(${BINARY_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/target/$<CONFIG>"
    ## Visual Studio specific
    # VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${BINARY_NAME}>"
    VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${BINARY_NAME}>"
    # VS_DEBUGGER_ENVIRONMENT       "PATH=%PATH%"
)
message(STATUS "Set ${BINARY_NAME} binary output directory to ${CMAKE_SOURCE_DIR}/target")


#===== Copy Commands ===================

# cmake build time copy
# file(GLOB EXTRA_DLLS_TO_COPY "${CMAKE_SOURCE_DIR}/lib/*.dll")
# add_custom_command(
#     TARGET ${BINARY_NAME}
#     POST_BUILD
#     COMMAND ${CMAKE_COMMAND}
#             -E copy_if_different
#             ${EXTRA_DLLS_TO_COPY}
#             $<TARGET_RUNTIME_DLLS:${BINARY_NAME}>
#             $<TARGET_FILE_DIR:${BINARY_NAME}> # target
#     COMMAND_EXPAND_LISTS
#     COMMENT "Copying dlls to target directory..."
# )


