if(NOT DEFINED WORD1_PATH OR WORD1_PATH STREQUAL "")
    message(FATAL_ERROR "WORD1_PATH is required")
endif()
if(NOT DEFINED ROUNDTRIP_DIR OR ROUNDTRIP_DIR STREQUAL "")
    message(FATAL_ERROR "ROUNDTRIP_DIR is required")
endif()

file(MAKE_DIRECTORY "${ROUNDTRIP_DIR}")
set(input_doc "${ROUNDTRIP_DIR}/word1-open-save-input.doc")
set(output_doc "${ROUNDTRIP_DIR}/word1-open-save-output.doc")
file(REMOVE "${input_doc}" "${output_doc}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        OPUS_HEADLESS=1
        SDL_VIDEODRIVER=dummy
        "${WORD1_PATH}"
        "--scripted-save-as-output=${input_doc}"
    RESULT_VARIABLE save_result
)
if(NOT save_result EQUAL 0)
    message(FATAL_ERROR "scripted Save As failed with ${save_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        OPUS_HEADLESS=1
        SDL_VIDEODRIVER=dummy
        "${WORD1_PATH}"
        "${input_doc}"
        "--scripted-save-as-output=${output_doc}"
    RESULT_VARIABLE roundtrip_result
)
if(NOT roundtrip_result EQUAL 0)
    message(FATAL_ERROR "scripted command-line open/save failed with ${roundtrip_result}")
endif()

file(SHA256 "${input_doc}" input_hash)
file(SHA256 "${output_doc}" output_hash)
if(NOT input_hash STREQUAL output_hash)
    message(FATAL_ERROR
        "roundtrip bytes differ: ${input_hash} != ${output_hash}")
endif()
