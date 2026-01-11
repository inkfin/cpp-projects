add_library(webgpu_glfw_backend INTERFACE)

if(EMSCRIPTEN)

    target_link_options(webgpu_glfw_backend
        INTERFACE
            --use-port=emdawnwebgpu
            -sASYNCIFY=1
            -sUSE_GLFW=3
            # -sALLOW_MEMORY_GROWTH=1
            # -sMODULARIZE=1
            # -sEXPORT_ES6=1
            # -sENVIRONMENT=web
    )

    target_link_libraries(webgpu_glfw_backend
        INTERFACE
            webgpu_dawn emscripten_glfw
    )

else() # Not EMSCRIPTEN

    set(DAWN_CLONE_DIR "${CMAKE_SOURCE_DIR}/../dawn" CACHE PATH
        "Path to local Dawn clone, default to ${CMAKE_SOURCE_DIR}/../dawn"
    )
    set(DAWN_VERSION "HEAD" CACHE STRING
        "Dawn git tag/commit/branch, default to HEAD"
    )

    include(FetchContent)
    include(FindPython3)

    FetchContent_Declare(dawn
        GIT_REPOSITORY https://github.com/google/dawn.git
        GIT_TAG        ${DAWN_VERSION}
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
        SOURCE_DIR     ${DAWN_CLONE_DIR}

        EXCLUDE_FROM_ALL
    )
    FetchContent_MakeAvailable(dawn)

    if("${dawn_SOURCE_DIR}")
    # Manually fetch dependencies of Dawn because Dawn's third_party module
    # won't shallow clone them by default with DAWN_FETCH_DEPENDENCIES.
    message(STATUS "Running fetch_dawn_dependencies:")
    execute_process(
        COMMAND
            ${Python3_EXECUTABLE}
            "${dawn_SOURCE_DIR}/tools/fetch_dawn_dependencies.py"
            --directory ${dawn_SOURCE_DIR}
            -s  # Shallow clone
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL "0")
        message(FATAL_ERROR "fetch_dawn_dependencies.py failed with exit code ${result}")
    endif()
    endif()

    find_package(glfw3 CONFIG REQUIRED)

    if(APPLE)
        find_library(COREFOUNDATION_FRAMEWORK CoreFoundation REQUIRED)
        find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
        find_library(METAL_FRAMEWORK Metal REQUIRED)
        find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
        find_library(IOSURFACE_FRAMEWORK IOSurface REQUIRED)
        find_library(IOKIT_FRAMEWORK IOKit REQUIRED)

        target_link_libraries(webgpu_glfw_backend
            INTERFACE
                ${COREFOUNDATION_FRAMEWORK}
                ${FOUNDATION_FRAMEWORK}
                ${METAL_FRAMEWORK}
                ${QUARTZCORE_FRAMEWORK}
                ${IOSURFACE_FRAMEWORK}
                ${IOKIT_FRAMEWORK}
        )
    endif()

    target_link_libraries(webgpu_glfw_backend
        INTERFACE
            webgpu_dawn glfw
    )

endif()
