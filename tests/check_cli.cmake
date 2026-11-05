set(usage "Usage: vna [--help | --version]\n       vna <ip> [port] [--prompt]\n\nConnect over TCP, print the *IDN? response, then exit.\n\nOptions:\n  -h, --help     Show this help message.\n  -v, --version  Show the library version.\n  --prompt       Enter interactive mode after *IDN? (.exit/exit/quit to leave).\n\nThe default port is 5025. IP must be a numeric IPv4 or IPv6 address.\n")

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
check_cli(1 "" "Error: unsupported arguments.\n${usage}" --prompt)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" 127.0.0.1 0)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" 127.0.0.1 65536)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" 127.0.0.1 5x)
check_cli(1 "" "Error: unsupported arguments.\n${usage}" 127.0.0.1 --prompt --prompt)
