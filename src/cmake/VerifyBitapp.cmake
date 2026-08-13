foreach(required_var IN ITEMS BITAPP INPUT OUTPUT EXPECTED WORKDIR)
    if(NOT DEFINED ${required_var})
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

get_filename_component(output_dir "${OUTPUT}" DIRECTORY)
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${output_dir}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE make_dir_result
)
if(NOT make_dir_result EQUAL 0)
    message(FATAL_ERROR "Could not create BITAPP output directory")
endif()

execute_process(
    COMMAND "${BITAPP}" "${INPUT}" "${OUTPUT}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE bitapp_result
)
if(NOT bitapp_result EQUAL 0)
    message(FATAL_ERROR "BITAPP failed with ${bitapp_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${OUTPUT}" "${EXPECTED}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE compare_result
)
if(NOT compare_result EQUAL 0)
    message(FATAL_ERROR "BITAPP output differs from ${EXPECTED}")
endif()
