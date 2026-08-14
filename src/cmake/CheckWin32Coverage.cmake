if(POLICY CMP0057)
    cmake_policy(SET CMP0057 NEW)
endif()

foreach(required_var IN ITEMS NM ARCHIVES HEADER BASELINE)
    if(NOT DEFINED ${required_var} OR "${${required_var}}" STREQUAL "")
        message(FATAL_ERROR "${required_var} is required")
    endif()
endforeach()

file(STRINGS "${HEADER}" header_lines)
set(declared)
foreach(line IN LISTS header_lines)
    string(REGEX MATCH
        "^[ \t]*(extern[ \t]+)?[A-Za-z_][A-Za-z0-9_ \t\\*]*[ \t\\*]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*\\("
        declaration_match "${line}")
    if(declaration_match)
        list(APPEND declared "${CMAKE_MATCH_2}")
    endif()
    string(REGEX MATCH
        "^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]+([A-Za-z_][A-Za-z0-9_]*)[ \t]*$"
        alias_match "${line}")
    if(alias_match)
        list(APPEND declared "${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
    endif()
endforeach()
list(REMOVE_DUPLICATES declared)
if(declared STREQUAL "")
    message(FATAL_ERROR "No Win32 declarations found in ${HEADER}")
endif()

set(undefined)
set(defined)
foreach(archive IN LISTS ARCHIVES)
    execute_process(
        COMMAND "${NM}" -g "${archive}"
        RESULT_VARIABLE nm_result
        OUTPUT_VARIABLE nm_output
        ERROR_VARIABLE nm_error
    )
    if(NOT nm_result EQUAL 0 AND nm_output STREQUAL "")
        message(FATAL_ERROR "nm failed for ${archive}: ${nm_error}")
    endif()
    string(REPLACE "\n" ";" nm_lines "${nm_output}")
    foreach(line IN LISTS nm_lines)
        string(STRIP "${line}" line)
        string(REGEX MATCH "(^|[ \t])U[ \t]+_?([A-Za-z][A-Za-z0-9_]*)$"
            undefined_match "${line}")
        if(undefined_match)
            list(APPEND undefined "${CMAKE_MATCH_2}")
        else()
            string(REGEX MATCH
                "(^|[0-9A-Fa-f]+[ \t]+)[A-Za-z][ \t]+_?([A-Za-z][A-Za-z0-9_]*)$"
                defined_match "${line}")
            if(defined_match)
                list(APPEND defined "${CMAKE_MATCH_2}")
            endif()
        endif()
    endforeach()
endforeach()
list(REMOVE_DUPLICATES undefined)
list(REMOVE_DUPLICATES defined)

set(uncovered)
foreach(name IN LISTS undefined)
    if(name IN_LIST declared AND NOT name IN_LIST defined)
        list(APPEND uncovered "${name}")
    endif()
endforeach()
list(REMOVE_DUPLICATES uncovered)
list(SORT uncovered)

if(DEFINED UPDATE_BASELINE AND UPDATE_BASELINE)
    string(REPLACE ";" "\n" baseline_text "${uncovered}")
    if(NOT baseline_text STREQUAL "")
        string(APPEND baseline_text "\n")
    endif()
    file(WRITE "${BASELINE}" "${baseline_text}")
    return()
endif()

set(baseline)
if(EXISTS "${BASELINE}")
    file(STRINGS "${BASELINE}" baseline)
endif()
list(SORT baseline)

set(new_uncovered "${uncovered}")
list(REMOVE_ITEM new_uncovered ${baseline})
set(stale_baseline "${baseline}")
list(REMOVE_ITEM stale_baseline ${uncovered})

if(NOT new_uncovered STREQUAL "" OR NOT stale_baseline STREQUAL "")
    set(message_text "Win32 coverage drift detected")
    if(NOT new_uncovered STREQUAL "")
        string(REPLACE ";" "\n  " new_text "${new_uncovered}")
        string(APPEND message_text "\nNew uncovered entry points:\n  ${new_text}")
    endif()
    if(NOT stale_baseline STREQUAL "")
        string(REPLACE ";" "\n  " stale_text "${stale_baseline}")
        string(APPEND message_text "\nStale baseline entries now covered:\n  ${stale_text}")
    endif()
    message(FATAL_ERROR "${message_text}")
endif()
