#ifndef LIBVNASCPI_UTILITY_H
#define LIBVNASCPI_UTILITY_H

#if defined(LIBVNASCPI_SHARED)
#if defined(_WIN32) || defined(__CYGWIN__)
#if defined(LIBVNASCPI_BUILDING_LIBRARY)
#define LIBVNASCPI_API __declspec(dllexport)
#else
#define LIBVNASCPI_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) && __GNUC__ >= 4
#define LIBVNASCPI_API __attribute__((visibility("default")))
#else
#define LIBVNASCPI_API
#endif
#else
#define LIBVNASCPI_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Operations return a status and leave output parameters unchanged on error. */
typedef enum vna_status {
    VNA_STATUS_SUCCESS = 0,
    VNA_STATUS_INVALID_ARGUMENT,
    VNA_STATUS_OUT_OF_RANGE,
    VNA_STATUS_NO_MEMORY,
    VNA_STATUS_CONNECTION_ERROR,
    VNA_STATUS_TIMEOUT,
    VNA_STATUS_IO_ERROR
} vna_status;

LIBVNASCPI_API const char *vna_status_string(vna_status status);

#ifdef __cplusplus
}
#endif

#endif
