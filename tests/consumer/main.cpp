#include <libvna/vna.h>

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
    return EXIT_SUCCESS;
}
