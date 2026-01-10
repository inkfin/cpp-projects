function(set_binary_output_dirs TARGET OUTPUT_BASE)
    set(_outdir "${CMAKE_SOURCE_DIR}/${OUTPUT_BASE}")

    set_target_properties(${TARGET} PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY "${_outdir}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${_outdir}/$<CONFIG>"
        RUNTIME_OUTPUT_DIRECTORY "${_outdir}/$<CONFIG>"
    )

    if(MSVC)
        ## Visual Studio specific
        set_target_properties(${TARGET} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${TARGET}>"
            VS_DEBUGGER_COMMAND           "$<TARGET_FILE:${TARGET}>"
        )
    endif()

    message(STATUS "Set ${TARGET} binary output directory to ${_outdir}")
endfunction()

