#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "libvna/vna.h"

static void print_usage(FILE *stream)
{
    fputs("Usage: vna [--help | --version]\n"
          "       vna <ip> [port] [--prompt]\n"
          "\n"
          "Connect over TCP, print the *IDN? response, then exit.\n"
          "\n"
          "Options:\n"
          "  -h, --help     Show this help message.\n"
          "  -v, --version  Show the library version.\n"
          "  --prompt       Enter interactive mode after *IDN? (exit/quit to leave).\n"
          "\n"
          "The default port is 5025. IP must be a numeric IPv4 or IPv6 address.\n",
          stream);
}

static int parse_port(const char *text, unsigned int *port)
{
    unsigned int value = 0;
    if (*text == '\0')
        return 0;
    for (; *text != '\0'; ++text) {
        if (*text < '0' || *text > '9')
            return 0;
        value = value * 10 + (unsigned int)(*text - '0');
        if (value > 65535)
            return 0;
    }
    if (value == 0)
        return 0;
    *port = value;
    return 1;
}

static int print_query(vna_connection *connection, const char *command)
{
    char *response;
    vna_status status = vna_query(connection, command, &response);
    if (status != VNA_STATUS_SUCCESS) {
        fprintf(stderr, "Error: query failed: %s.\n", vna_status_string(status));
        return 0;
    }
    puts(response);
    vna_free_response(response);
    return 1;
}

static int is_query(const char *command)
{
    char quote = '\0';
    for (; *command != '\0'; ++command) {
        if (quote != '\0') {
            if (*command == quote) {
                if (command[1] == quote)
                    ++command;
                else
                    quote = '\0';
            }
        } else if (*command == '\'' || *command == '"') {
            quote = *command;
        } else if (*command == '?') {
            return 1;
        }
    }
    return 0;
}

static int prompt(vna_connection *connection)
{
    char *line = NULL;
    size_t capacity = 0;
    int success = 1;
    for (;;) {
        size_t length = 0;
        int ch;
        fputs("vna> ", stdout);
        fflush(stdout);
        while ((ch = fgetc(stdin)) != '\n' && ch != EOF) {
            if (length + 1 >= capacity) {
                size_t next = capacity == 0 ? 256 :
                    (capacity <= SIZE_MAX / 2 ? capacity * 2 : 0);
                char *grown = next == 0 ? NULL : realloc(line, next);
                if (grown == NULL) {
                    fputs("Error: input line is too large or out of memory.\n", stderr);
                    free(line);
                    return 0;
                }
                line = grown;
                capacity = next;
            }
            if (ch == '\0') {
                fputs("Error: input contains a NUL byte.\n", stderr);
                free(line);
                return 0;
            }
            line[length++] = (char)ch;
        }
        if (ferror(stdin)) {
            fputs("Error: could not read standard input.\n", stderr);
            success = 0;
            break;
        }
        if (length == 0 && ch == EOF)
            break;
        if (length > 0 && line[length - 1] == '\r')
            --length;
        if (length == 0)
            continue;
        line[length] = '\0';
        if (strcmp(line, "exit") == 0 || strcmp(line, "quit") == 0)
            break;
        if (is_query(line)) {
            success = print_query(connection, line);
        } else {
            vna_status status = vna_send(connection, line);
            success = status == VNA_STATUS_SUCCESS;
            if (!success)
                fprintf(stderr, "Error: command failed: %s.\n",
                        vna_status_string(status));
        }
        if (!success)
            break;
    }
    free(line);
    return success;
}

int main(int argc, char *argv[])
{
    const char *ip = NULL;
    unsigned int port = VNA_DEFAULT_PORT;
    int have_port = 0;
    int interactive = 0;
    int success;
    vna_status status;
    vna_connection *connection;

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

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--prompt") == 0 && !interactive) {
            interactive = 1;
        } else if (argv[i][0] == '-') {
            goto invalid_arguments;
        } else if (ip == NULL) {
            ip = argv[i];
        } else if (!have_port && parse_port(argv[i], &port)) {
            have_port = 1;
        } else {
            goto invalid_arguments;
        }
    }
    if (ip == NULL)
        goto invalid_arguments;
    connection = vna_connect(ip, port, &status);
    if (connection == NULL) {
        fprintf(stderr, "Error: connection failed: %s.\n", vna_status_string(status));
        return EXIT_FAILURE;
    }
    success = print_query(connection, "*IDN?");
    if (success && interactive)
        success = prompt(connection);
    vna_disconnect(connection);
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fputs("Error: could not write standard output.\n", stderr);
        success = 0;
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;

invalid_arguments:
    fputs("Error: unsupported arguments.\n", stderr);
    print_usage(stderr);
    return EXIT_FAILURE;
}
