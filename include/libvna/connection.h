#ifndef LIBVNA_CONNECTION_H
#define LIBVNA_CONNECTION_H

#include <libvna/utility.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VNA_DEFAULT_PORT 5025
#define VNA_TIMEOUT_MS 5000

typedef struct vna_connection vna_connection;

/* Numeric IPv4/IPv6 only. Port 0 selects VNA_DEFAULT_PORT.
 * Returns NULL on failure. Optional status receives the operation result. */
LIBVNA_API vna_connection *vna_connect(const char *ip, unsigned int port,
                                     vna_status *status);
LIBVNA_API void vna_disconnect(vna_connection *connection);

/* Commands must be nonempty single lines without CR/LF; LF is appended. */
LIBVNA_API vna_status vna_send(vna_connection *connection, const char *command);

/* Sends a command and reads one LF-terminated text response, stripping CR/LF.
 * On success, release *response with vna_free_response. */
LIBVNA_API vna_status vna_query(vna_connection *connection, const char *command,
                              char **response);
LIBVNA_API void vna_free_response(char *response);

#ifdef __cplusplus
}
#endif

#endif
