cmake_minimum_required(VERSION 3.24)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" LIBVNA_PROJECT_VERSION)
string(STRIP "${LIBVNA_PROJECT_VERSION}" LIBVNA_PROJECT_VERSION)
if(NOT LIBVNA_PROJECT_VERSION MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR "VERSION must contain a major.minor.patch version")
endif()
project(libvna VERSION ${LIBVNA_PROJECT_VERSION}
    DESCRIPTION "SCPI vector network analyzer client" LANGUAGES C)

include(FetchContent)
option(BUILD_SHARED_LIBS "Build vna as a shared library" OFF)
option(BUILD_TESTING "Build and register the libvna tests" OFF)
option(LIBVNA_BUILD_CLI "Build the libvna command-line tool" ON)
include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(WIN32 AND DEFINED ENV{LOCALAPPDATA} AND NOT "$ENV{LOCALAPPDATA}" STREQUAL "")
        file(TO_CMAKE_PATH "$ENV{LOCALAPPDATA}/Programs/libvna" _default_install_prefix)
    elseif(UNIX AND DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
        set(_default_install_prefix "$ENV{HOME}/.local")
    endif()
    if(DEFINED _default_install_prefix)
        set(CMAKE_INSTALL_PREFIX "${_default_install_prefix}" CACHE PATH
            "Install prefix for libvna" FORCE)
        unset(_default_install_prefix)
    endif()
endif()

function(libvna_fetch_libuv)
    # libuv selects an alias using BUILD_SHARED_LIBS; keep its backend static
    # without changing the enclosing project's library selection.
    set(BUILD_SHARED_LIBS OFF)
    set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(libuv
        URL https://dist.libuv.org/dist/v1.51.0/libuv-v1.51.0.tar.gz
        URL_HASH SHA256=5f0557b90b1106de71951a3c3931de5e0430d78da1d9a10287ebc7a3f78ef8eb
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(libuv)
endfunction()
libvna_fetch_libuv()
set_target_properties(uv_a PROPERTIES POSITION_INDEPENDENT_CODE ON)

configure_file(src/version.h.in generated/libvna_version.h @ONLY)

add_library(vna src/version.c)
add_library(libvna::vna ALIAS vna)
target_compile_features(vna PUBLIC c_std_11)
set_target_properties(vna PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION}
    C_EXTENSIONS OFF
)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(vna PUBLIC LIBVNA_SHARED PRIVATE LIBVNA_BUILDING_LIBRARY)
    set_target_properties(vna PROPERTIES C_VISIBILITY_PRESET hidden)
endif()
target_include_directories(vna
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(vna PRIVATE uv_a)

if(LIBVNA_BUILD_CLI)
    add_executable(vna_cli cli/main.c)
    set_target_properties(vna_cli PROPERTIES
        OUTPUT_NAME vna
        ARCHIVE_OUTPUT_NAME vna_cli
        C_EXTENSIONS OFF
    )
    target_link_libraries(vna_cli PRIVATE libvna::vna)
endif()

install(TARGETS vna EXPORT libvnaTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
if(LIBVNA_BUILD_CLI)
    install(TARGETS vna_cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT libvnaTargets
    FILE libvnaTargets.cmake
    NAMESPACE libvna::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libvna
)
configure_package_config_file(cmake/libvnaConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/libvnaConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libvna
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/libvnaConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/libvnaConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/libvnaConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/libvna
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(CPACK_PACKAGE_VENDOR "AntennaMeasurement")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}")
if(WIN32)
    set(CPACK_GENERATOR ZIP)
else()
    set(CPACK_GENERATOR TGZ)
endif()
include(CPack)
