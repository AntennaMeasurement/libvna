#ifndef LIBVNA_VNA_H
#define LIBVNA_VNA_H

#include <libvna/utility.h>
#include <libvna/connection.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Returns the library version as a static string. Do not free it. */
LIBVNA_API const char *vna_version(void);

#ifdef __cplusplus
}
#endif

#endif
