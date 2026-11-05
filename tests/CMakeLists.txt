add_executable(vna_test_version test_version.c)
set_target_properties(vna_test_version PROPERTIES C_EXTENSIONS OFF)
target_link_libraries(vna_test_version PRIVATE libvna::vna uv_a)
target_compile_definitions(vna_test_version PRIVATE
    EXPECTED_VERSION="${PROJECT_VERSION}"
)
add_test(NAME vna.version_and_backend COMMAND vna_test_version)
set_tests_properties(vna.version_and_backend PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:vna>"
)

add_executable(vna_test_connection test_connection.c)
set_target_properties(vna_test_connection PROPERTIES C_EXTENSIONS OFF)
target_link_libraries(vna_test_connection PRIVATE libvna::vna)
add_test(NAME vna.connection_validation COMMAND vna_test_connection)
set_tests_properties(vna.connection_validation PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:vna>"
)

add_test(NAME vna.installed_package
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${PROJECT_SOURCE_DIR}"
        "-DBINARY_DIR=${PROJECT_BINARY_DIR}"
        "-DCONFIG=$<CONFIG>"
        "-DGENERATOR=${CMAKE_GENERATOR}"
        "-DPLATFORM=${CMAKE_GENERATOR_PLATFORM}"
        "-DTOOLSET=${CMAKE_GENERATOR_TOOLSET}"
        "-DEXPECTED_VERSION=${PROJECT_VERSION}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_package.cmake"
)

if(LIBVNA_BUILD_CLI)
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME vna.network
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/test_network.py"
            "$<TARGET_FILE:vna_test_connection>" "$<TARGET_FILE:vna_cli>"
    )
    set_tests_properties(vna.network PROPERTIES
        TIMEOUT 90
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:$<TARGET_FILE_DIR:vna>"
    )
    add_test(NAME vna.cli
        COMMAND "${CMAKE_COMMAND}"
            "-DCLI=$<TARGET_FILE:vna_cli>"
            "-DVERSION=${PROJECT_VERSION}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check_cli.cmake"
    )
endif()
