#include <libvnascpi/vna.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void)
{
    const char *version = vna_version();

    if (version == NULL || strcmp(version, EXPECTED_VERSION) != 0) {
        fputs("Unexpected installed library version.\n", stderr);
        return EXIT_FAILURE;
    }
    vna_status status;
    if (vna_connect(NULL, VNA_DEFAULT_PORT, &status) != NULL ||
        status != VNA_STATUS_INVALID_ARGUMENT ||
        strcmp(vna_status_string(status), "Invalid argument") != 0) {
        fputs("Unexpected installed connection API behavior.\n", stderr);
        return EXIT_FAILURE;
    }
    vna_disconnect(NULL);
    vna_free_response(NULL);
    return EXIT_SUCCESS;
}
