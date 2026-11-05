add_executable(vna_test_version test_version.c)
set_target_properties(vna_test_version PROPERTIES C_EXTENSIONS OFF)
target_link_libraries(vna_test_version PRIVATE libvna::vna uv_a)
target_compile_definitions(vna_test_version PRIVATE
    EXPECTED_VERSION="${PROJECT_VERSION}"
)
add_test(NAME vna.version_and_backend COMMAND vna_test_version)

if(LIBVNA_BUILD_CLI)
    add_test(NAME vna.cli
        COMMAND "${CMAKE_COMMAND}"
            "-DCLI=$<TARGET_FILE:vna_cli>"
            "-DVERSION=${PROJECT_VERSION}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check_cli.cmake"
    )
endif()
