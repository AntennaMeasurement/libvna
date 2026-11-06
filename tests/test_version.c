#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>

#include "libvnascpi/vna.h"

int main(void)
{
    const char *version = vna_version();
    uv_loop_t loop;
    int status;

    if (version == NULL || strcmp(version, EXPECTED_VERSION) != 0) {
        fputs("Unexpected library version.\n", stderr);
        return EXIT_FAILURE;
    }

    status = uv_loop_init(&loop);
    if (status != 0) {
        fprintf(stderr, "uv_loop_init: %s\n", uv_strerror(status));
        return EXIT_FAILURE;
    }

    status = uv_loop_close(&loop);
    if (status != 0) {
        fprintf(stderr, "uv_loop_close: %s\n", uv_strerror(status));
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
