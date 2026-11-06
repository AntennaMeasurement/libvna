# C API

Include the umbrella header:

```c
#include <libvnascpi/vna.h>

const char *version = vna_version();
```

`vna_version()` returns a static,
library-owned string; callers must not free it. The API supports C and C++
callers and does not expose libuv headers.

`libvnascpi/utility.h` provides `LIBVNASCPI_API` for symbol visibility and the common
`vna_status` type:

| Status | Meaning |
| --- | --- |
| `VNA_STATUS_SUCCESS` | Operation completed successfully |
| `VNA_STATUS_INVALID_ARGUMENT` | An input argument is invalid |
| `VNA_STATUS_OUT_OF_RANGE` | A value/result is outside its supported range |
| `VNA_STATUS_NO_MEMORY` | An allocation failed |
| `VNA_STATUS_CONNECTION_ERROR` | TCP connection could not be established |
| `VNA_STATUS_TIMEOUT` | A network operation exceeded its deadline |
| `VNA_STATUS_IO_ERROR` | Connection closed or a read/write failed |

`vna_status_string(status)` returns a static, library-owned description.
Status-returning APIs leave output parameters unchanged on error.

## TCP connection and SCPI I/O

These APIs are declared in `libvnascpi/connection.h`, also included by `vna.h`.

```c
vna_connection *vna_connect(const char *ip, unsigned int port, vna_status *status);
void vna_disconnect(vna_connection *connection);
vna_status vna_send(vna_connection *connection, const char *command);
vna_status vna_query(vna_connection *connection, const char *command, char **response);
void vna_free_response(char *response);
vna_status vna_query_block(vna_connection *connection, const char *command,
                           unsigned char **data, size_t *length);
void vna_free_data(void *data);
```

`vna_connect()` synchronously connects to a numeric IPv4/IPv6 address and
returns an opaque, library-owned connection pointer. Hostnames are not
accepted. Port `0` selects `VNA_DEFAULT_PORT` (**5025**); ports above 65535
are rejected. On failure it returns `NULL`. If `status` is non-NULL, it is
always set to the result, including success.

`vna_send()` appends LF and writes a nonempty command. Commands containing
CR/LF are rejected, so pass commands without their line terminator.
`vna_query()` sends the command, reads one LF-terminated text response, strips
the LF and optional preceding CR, and assigns an allocated string to
`*response` on success. Free that string with `vna_free_response()`, including
when using a shared library. Text responses may be empty; binary blocks and
multiline responses are not supported. Responses grow dynamically; fragments
are reassembled and any bytes beyond the first LF are retained.

Connect, write, and response-read stages each have a **5000 ms** timeout
(`VNA_TIMEOUT_MS`). A query may therefore spend up to 5 seconds writing and
another 5 seconds reading. Timeout, read/write failure, or failure to retain
a response makes the connection unusable, preventing reuse of a partially
consumed stream. Disconnect it and establish a new connection. Invalid input
and command-allocation failures before transmission leave the connection
usable.

`vna_query_block()` sends a query and reads an IEEE 488.2 definite-length
binary block (`#<digit-count><byte-count><payload>`) followed by LF or CRLF.
It returns the exact payload bytes and length; binary data may contain any
byte value, including NUL and LF. The block length header must use 1–9 digits;
indefinite-length blocks are not supported. Release the payload with
`vna_free_data()`. The response-read timeout applies while receiving the
header, payload, and line terminator. Malformed or incomplete responses make
the connection unusable, as with failed text queries.

For example, to retrieve a VNA screen image saved on the instrument:

```c
unsigned char *image;
size_t image_length;
status = vna_query_block(device, "MMEM:DATA? \"C:\\screen.png\"",
                         &image, &image_length);
if (status == VNA_STATUS_SUCCESS) {
    FILE *file = fopen("screen.png", "wb");
    if (file == NULL || fwrite(image, 1, image_length, file) != image_length) {
        /* Report the local file write failure. */
    }
    if (file != NULL)
        fclose(file);
    vna_free_data(image);
}
```

The example also requires `<stdio.h>`. Check the status and local file
operations in production code; instrument-specific file paths and image-save
commands may vary.

Always release a successful connection with `vna_disconnect()`.
`vna_disconnect(NULL)` and `vna_free_response(NULL)` are safe. Do not use a
connection after disconnecting it. Calls on the same connection must not
overlap; separate connections have independent private event loops.

```c
vna_status status;
vna_connection *device = vna_connect("192.168.1.100", 0, &status);
if (device == NULL) {
    fprintf(stderr, "Connect: %s\n", vna_status_string(status));
    return 1;
}

char *response;
status = vna_query(device, "*IDN?", &response);
if (status == VNA_STATUS_SUCCESS) {
    puts(response);
    vna_free_response(response);
} else {
    fprintf(stderr, "Query: %s\n", vna_status_string(status));
}
vna_disconnect(device);
```

The example also requires `<stdio.h>`.

When linking a shared build manually, consumers must define `LIBVNASCPI_SHARED`.
The exported CMake target propagates this automatically. Only the library
build defines `LIBVNASCPI_BUILDING_LIBRARY`.

Implementations belong in module-specific sources under `src`; public
module headers belong under `include/libvnascpi` and are included by the umbrella
header when introduced. Version implementation is isolated in `src/version.c`.
`VERSION` is the single source for the generated version header.
