function(run_checked)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE status)
    if(NOT "${status}" STREQUAL "0")
        message(FATAL_ERROR "Package check failed (${status}): ${ARGV}")
    endif()
endfunction()

foreach(required IN ITEMS SOURCE_DIR BINARY_DIR GENERATOR EXPECTED_VERSION)
    if(NOT DEFINED ${required} OR "${${required}}" STREQUAL "")
        message(FATAL_ERROR "${required} is required for the installed-package check")
    endif()
endforeach()
if(NOT IS_ABSOLUTE "${BINARY_DIR}")
    message(FATAL_ERROR "BINARY_DIR must be absolute")
endif()

set(config_args)
set(ctest_config_args)
if(NOT "${CONFIG}" STREQUAL "")
    list(APPEND config_args --config "${CONFIG}")
    list(APPEND ctest_config_args -C "${CONFIG}")
endif()
set(test_root "${BINARY_DIR}/tests/installed-package")
file(REMOVE_RECURSE "${test_root}")
set(prefix "${test_root}/stage")
set(relocated_prefix "${test_root}/relocated")
run_checked("${CMAKE_COMMAND}" --install "${BINARY_DIR}"
    ${config_args} --prefix "${prefix}")
file(RENAME "${prefix}" "${relocated_prefix}")

set(generator_args -G "${GENERATOR}")
if(NOT "${PLATFORM}" STREQUAL "")
    list(APPEND generator_args -A "${PLATFORM}")
endif()
if(NOT "${TOOLSET}" STREQUAL "")
    list(APPEND generator_args -T "${TOOLSET}")
endif()
run_checked("${CMAKE_COMMAND}" -S "${SOURCE_DIR}/tests/consumer"
    -B "${test_root}/consumer" ${generator_args}
    "-DCMAKE_PREFIX_PATH=${relocated_prefix}"
    "-DCMAKE_BUILD_TYPE=${CONFIG}"
    "-DLIBVNASCPI_RUNTIME_DIR=${relocated_prefix}/bin"
    "-DEXPECTED_VERSION=${EXPECTED_VERSION}")
run_checked("${CMAKE_COMMAND}" --build "${test_root}/consumer" ${config_args})
run_checked("${CMAKE_CTEST_COMMAND}" --test-dir "${test_root}/consumer"
    ${ctest_config_args} --output-on-failure)
