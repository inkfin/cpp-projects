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
else()
    add_subdirectory(path/to/dawn)  # 生成 webgpu_dawn/webgpu_glfw/glfw
    target_link_libraries(webgpu_glfw_backend INTERFACE
        webgpu_dawn webgpu_glfw glfw
    )
endif()
