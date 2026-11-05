set(usage "Usage: vna [--help | --version]\n\nSCPI vector network analyzer client (project skeleton).\nTCP connections and SCPI commands are not implemented yet.\n\nOptions:\n  -h, --help     Show this help message.\n  -v, --version  Show the library version.\n")

function(check_cli expected_status expected_stdout expected_stderr)
    execute_process(
        COMMAND "${CLI}" ${ARGN}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
        TIMEOUT 10
    )
    if(NOT "${status}" STREQUAL "${expected_status}")
        message(FATAL_ERROR "vna ${ARGN}: expected status ${expected_status}, got ${status}: ${error}")
    endif()
    if(NOT "${output}" STREQUAL "${expected_stdout}")
        message(FATAL_ERROR "vna ${ARGN}: unexpected stdout: ${output}")
    endif()
    if(NOT "${error}" STREQUAL "${expected_stderr}")
        message(FATAL_ERROR "vna ${ARGN}: unexpected stderr: ${error}")
    endif()
endfunction()

check_cli(0 "${usage}" "")
check_cli(0 "${usage}" "" --help)
check_cli(0 "${usage}" "" -h)
check_cli(0 "libvna ${VERSION}\n" "" --version)
check_cli(0 "libvna ${VERSION}\n" "" -v)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" --unknown)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" --version extra)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" --help extra)
