cmake_minimum_required(VERSION 3.24)

project(libvna VERSION 0.1.0 DESCRIPTION "SCPI vector network analyzer client" LANGUAGES C)

include(FetchContent)
include(CTest)

option(LIBVNA_BUILD_CLI "Build the libvna command-line tool" ON)

set(LIBUV_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LIBUV_BUILD_BENCH OFF CACHE BOOL "" FORCE)

FetchContent_Declare(libuv
    URL https://dist.libuv.org/dist/v1.51.0/libuv-v1.51.0.tar.gz
    URL_HASH SHA256=5f0557b90b1106de71951a3c3931de5e0430d78da1d9a10287ebc7a3f78ef8eb
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(libuv)

configure_file(src/version.h.in generated/libvna_version.h @ONLY)

add_library(vna STATIC src/vna.c)
add_library(libvna::vna ALIAS vna)
target_compile_features(vna PUBLIC c_std_11)
set_target_properties(vna PROPERTIES C_EXTENSIONS OFF)
target_include_directories(vna
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(vna PRIVATE uv_a)

if(LIBVNA_BUILD_CLI)
    add_executable(vna_cli cli/main.c)
    set_target_properties(vna_cli PROPERTIES OUTPUT_NAME vna C_EXTENSIONS OFF)
    target_link_libraries(vna_cli PRIVATE libvna::vna)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
