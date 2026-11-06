cmake_minimum_required(VERSION 3.24)
project(libvnascpi_consumer LANGUAGES C CXX)

enable_testing()
find_package(libvnascpi "${EXPECTED_VERSION}" EXACT CONFIG REQUIRED)
foreach(language IN ITEMS c cpp)
    add_executable(consumer_${language} "main.${language}")
    target_link_libraries(consumer_${language} PRIVATE libvnascpi::vna)
    target_compile_definitions(consumer_${language} PRIVATE
        EXPECTED_VERSION="${EXPECTED_VERSION}")
    add_test(NAME consumer_${language} COMMAND consumer_${language})
    set_tests_properties(consumer_${language} PROPERTIES
        ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${LIBVNASCPI_RUNTIME_DIR}"
    )
endforeach()
