#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libvna/vna.h"

static void print_usage(FILE *stream)
{
    fputs("Usage: vna [--help | --version]\n"
          "\n"
          "SCPI vector network analyzer client (project skeleton).\n"
          "TCP connections and SCPI commands are not implemented yet.\n"
          "\n"
          "Options:\n"
          "  -h, --help     Show this help message.\n"
          "  -v, --version  Show the library version.\n",
          stream);
}

int main(int argc, char *argv[])
{
    if (argc == 1 ||
        (argc == 2 &&
         (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))) {
        print_usage(stdout);
        return EXIT_SUCCESS;
    }

    if (argc == 2 &&
        (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0)) {
        printf("libvna %s\n", vna_version());
        return EXIT_SUCCESS;
    }

    fputs("Error: unsupported arguments.\n", stderr);
    print_usage(stderr);
    return EXIT_FAILURE;
}
