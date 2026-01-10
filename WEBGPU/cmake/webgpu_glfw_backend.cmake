add_library(webgpu_glfw_backend INTERFACE)

if(EMSCRIPTEN)

    target_link_options(webgpu_glfw_backend INTERFACE
        --use-port=emdawnwebgpu
        -sUSE_GLFW=3
        # -sALLOW_MEMORY_GROWTH=1
        # -sMODULARIZE=1
        # -sEXPORT_ES6=1
        # -sENVIRONMENT=web
    )

else() # Not EMSCRIPTEN

    set(DAWN_CLONE_DIR "${CMAKE_SOURCE_DIR}/../dawn" CACHE PATH
        "Path to local Dawn clone, default to ${CMAKE_SOURCE_DIR}/../dawn"
    )
    set(DAWN_VERSION "HEAD" CACHE STRING
        "Dawn git tag/commit/branch, default to HEAD"
    )

    include(FetchContent)
    FetchContent_Declare(dawn
        GIT_REPOSITORY https://github.com/google/dawn.git
        GIT_TAG        ${DAWN_VERSION}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
        SOURCE_DIR     ${DAWN_CLONE_DIR}
    )
    FetchContent_MakeAvailable(dawn)

    target_link_libraries(webgpu_glfw_backend INTERFACE
            webgpu_dawn webgpu_glfw glfw
        )

endif()
