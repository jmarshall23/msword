if(NOT DEFINED WORD1_PATH OR WORD1_PATH STREQUAL "")
    message(FATAL_ERROR "WORD1_PATH is required")
endif()
if(NOT DEFINED ROUNDTRIP_DIR OR ROUNDTRIP_DIR STREQUAL "")
    message(FATAL_ERROR "ROUNDTRIP_DIR is required")
endif()
if(NOT DEFINED WORKDIR OR WORKDIR STREQUAL "")
    message(FATAL_ERROR "WORKDIR is required")
endif()

file(MAKE_DIRECTORY "${ROUNDTRIP_DIR}")
set(input_doc "${ROUNDTRIP_DIR}/word1-open-save-input.doc")
set(output_doc "${ROUNDTRIP_DIR}/word1-open-save-output.doc")
set(sdl_video_driver "$ENV{SDL_VIDEODRIVER}")
if(sdl_video_driver STREQUAL "")
    set(sdl_video_driver offscreen)
endif()
file(REMOVE "${input_doc}" "${output_doc}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        OPUS_HEADLESS=1
        "SDL_VIDEODRIVER=${sdl_video_driver}"
        "${WORD1_PATH}"
        "--scripted-save-as-output=${input_doc}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE save_result
)
if(NOT save_result EQUAL 0)
    message(FATAL_ERROR "scripted Save As failed with ${save_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env
        OPUS_HEADLESS=1
        "SDL_VIDEODRIVER=${sdl_video_driver}"
        "${WORD1_PATH}"
        "${input_doc}"
        "--scripted-save-as-output=${output_doc}"
    WORKING_DIRECTORY "${WORKDIR}"
    RESULT_VARIABLE roundtrip_result
)
if(NOT roundtrip_result EQUAL 0)
    message(FATAL_ERROR "scripted command-line open/save failed with ${roundtrip_result}")
endif()

file(SIZE "${input_doc}" input_size)
file(SIZE "${output_doc}" output_size)
if(input_size EQUAL 0 OR output_size EQUAL 0)
    message(FATAL_ERROR
        "roundtrip produced empty file: ${input_size}, ${output_size}")
endif()
