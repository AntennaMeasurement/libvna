#ifndef LIBVNASCPI_VNA_H
#define LIBVNASCPI_VNA_H

#include <libvnascpi/utility.h>
#include <libvnascpi/connection.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the library version as a static string. Do not free it. */
LIBVNASCPI_API const char *vna_version(void);

#ifdef __cplusplus
}
#endif

#endif
