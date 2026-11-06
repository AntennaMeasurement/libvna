#include <libvnascpi/vna.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

int main()
{
    const char *version = vna_version();

    if (version == nullptr || std::strcmp(version, EXPECTED_VERSION) != 0) {
        std::fputs("Unexpected installed C++ library version.\n", stderr);
        return EXIT_FAILURE;
    }
    vna_status status;
    if (vna_connect(nullptr, VNA_DEFAULT_PORT, &status) != nullptr ||
        status != VNA_STATUS_INVALID_ARGUMENT ||
        std::strcmp(vna_status_string(status), "Invalid argument") != 0) {
        std::fputs("Unexpected installed C++ connection API behavior.\n", stderr);
        return EXIT_FAILURE;
    }
    vna_disconnect(nullptr);
    vna_free_response(nullptr);
    return EXIT_SUCCESS;
}
