#include <libvna/vna.h>

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
    return EXIT_SUCCESS;
}
