#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <conio.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

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
          "  --prompt       Enter interactive mode after *IDN? "
          "(.exit/exit/quit to leave).\n"
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

typedef struct {
    char **lines;
    size_t count;
    size_t capacity;
} command_history;

static int ensure_line_capacity(char **line, size_t *capacity, size_t needed)
{
    size_t next;
    char *grown;

    if (needed <= *capacity)
        return 1;
    next = *capacity == 0 ? 256 :
        (*capacity <= SIZE_MAX / 2 ? *capacity * 2 : 0);
    while (next < needed && next != 0)
        next = next <= SIZE_MAX / 2 ? next * 2 : 0;
    grown = next == 0 ? NULL : realloc(*line, next);
    if (grown == NULL)
        return 0;
    *line = grown;
    *capacity = next;
    return 1;
}

static int save_history(command_history *history, const char *line)
{
    char *copy;

    if (history->count == history->capacity) {
        size_t next = history->capacity == 0 ? 16 :
            (history->capacity <= SIZE_MAX / 2 ? history->capacity * 2 : 0);
        char **grown = next == 0 ||
            next > SIZE_MAX / sizeof(*history->lines) ? NULL :
            realloc(history->lines, next * sizeof(*history->lines));
        if (grown == NULL)
            return 0;
        history->lines = grown;
        history->capacity = next;
    }
    copy = malloc(strlen(line) + 1);
    if (copy == NULL)
        return 0;
    strcpy(copy, line);
    history->lines[history->count++] = copy;
    return 1;
}

static void free_history(command_history *history)
{
    for (size_t i = 0; i < history->count; ++i)
        free(history->lines[i]);
    free(history->lines);
}

static int terminal_key(void)
{
#ifdef _WIN32
    int key = _getch();
    if (key == 0 || key == 0xe0) {
        key = _getch();
        if (key == 72)
            return 0x100; /* Up */
        if (key == 80)
            return 0x101; /* Down */
        return -2;
    }
    return key;
#else
    int key = getchar();
    if (key != 0x1b)
        return key;
    key = getchar();
    if (key != '[')
        return -2;
    key = getchar();
    if (key == 'A')
        return 0x100; /* Up */
    if (key == 'B')
        return 0x101; /* Down */
    return -2;
#endif
}

static void redraw_line(const char *line, size_t length, size_t old_length)
{
    fputs("\rvna> ", stdout);
    if (length != 0)
        fwrite(line, 1, length, stdout);
    for (size_t i = length; i < old_length; ++i)
        putchar(' ');
    fputs("\rvna> ", stdout);
    if (length != 0)
        fwrite(line, 1, length, stdout);
    fflush(stdout);
}

static int read_terminal_line(char **line, size_t *length, size_t *capacity,
                              command_history *history, int *at_eof)
{
    size_t history_position = history->count;
    char *draft = NULL;
    size_t draft_length = 0;
    size_t draft_capacity = 0;
    int browsing_history = 0;
    int result = 1;

    *length = 0;
    *at_eof = 0;
    for (;;) {
        int key = terminal_key();
        if (key == EOF) {
            if (ferror(stdin))
                result = -1;
            else
                *at_eof = 1;
            break;
        }
        if (key == '\r' || key == '\n') {
            putchar('\n');
            break;
        }
        if (key == 4 && *length == 0) {
            *at_eof = 1;
            break;
        }
        if (key == 0x100 || key == 0x101) {
            size_t old_length = *length;
            if (key == 0x100 && history_position > 0) {
                if (!browsing_history) {
                    if (!ensure_line_capacity(&draft, &draft_capacity,
                                              *length + 1)) {
                        result = 0;
                        break;
                    }
                    if (*length != 0)
                        memcpy(draft, *line, *length);
                    draft[*length] = '\0';
                    draft_length = *length;
                    browsing_history = 1;
                }
                --history_position;
                *length = strlen(history->lines[history_position]);
                if (!ensure_line_capacity(line, capacity, *length + 1)) {
                    result = 0;
                    break;
                }
                memcpy(*line, history->lines[history_position], *length + 1);
            } else if (key == 0x101 && browsing_history &&
                       history_position < history->count) {
                ++history_position;
                if (history_position == history->count) {
                    *length = draft_length;
                    if (!ensure_line_capacity(line, capacity, *length + 1)) {
                        result = 0;
                        break;
                    }
                    if (*length != 0)
                        memcpy(*line, draft, *length);
                    (*line)[*length] = '\0';
                    browsing_history = 0;
                } else {
                    *length = strlen(history->lines[history_position]);
                    if (!ensure_line_capacity(line, capacity, *length + 1)) {
                        result = 0;
                        break;
                    }
                    memcpy(*line, history->lines[history_position],
                           *length + 1);
                }
            }
            redraw_line(*line, *length, old_length);
            continue;
        }
        if (key == 8 || key == 127) {
            if (*length != 0) {
                --*length;
                browsing_history = 0;
                history_position = history->count;
                putchar('\b');
                putchar(' ');
                putchar('\b');
                fflush(stdout);
            }
            continue;
        }
        if (key >= 32 && key != 127) {
            if (!ensure_line_capacity(line, capacity, *length + 2)) {
                result = 0;
                break;
            }
            (*line)[(*length)++] = (char)key;
            (*line)[*length] = '\0';
            browsing_history = 0;
            history_position = history->count;
            putchar(key);
            fflush(stdout);
        }
    }
    free(draft);
    return result;
}

static int prompt(vna_connection *connection)
{
    char *line = NULL;
    size_t capacity = 0;
    command_history history = { NULL, 0, 0 };
    int success = 1;
#ifdef _WIN32
    int terminal = _isatty(_fileno(stdin));
#else
    int terminal = isatty(STDIN_FILENO);
    struct termios original_settings;
    int settings_changed = 0;
    if (terminal) {
        struct termios settings;
        if (tcgetattr(STDIN_FILENO, &original_settings) != 0) {
            fputs("Error: could not configure terminal input.\n", stderr);
            return 0;
        }
        settings = original_settings;
        settings.c_lflag &= (tcflag_t) ~(ICANON | ECHO);
        settings.c_cc[VMIN] = 1;
        settings.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &settings) != 0) {
            fputs("Error: could not configure terminal input.\n", stderr);
            return 0;
        }
        settings_changed = 1;
    }
#endif
    for (;;) {
        size_t length = 0;
        int ch;
        int at_eof = 0;
        fputs("vna> ", stdout);
        fflush(stdout);
        if (terminal) {
            int read_result = read_terminal_line(
                &line, &length, &capacity, &history, &at_eof);
            if (read_result <= 0) {
                if (read_result < 0)
                    fputs("Error: could not read standard input.\n", stderr);
                else
                    fputs("Error: input line is too large or out of memory.\n",
                          stderr);
                success = 0;
                break;
            }
            ch = at_eof ? EOF : '\n';
        } else {
            while ((ch = fgetc(stdin)) != '\n' && ch != EOF) {
                if (!ensure_line_capacity(&line, &capacity, length + 2)) {
                    fputs("Error: input line is too large or out of memory.\n",
                          stderr);
                    success = 0;
                    goto done;
                }
                if (ch == '\0') {
                    fputs("Error: input contains a NUL byte.\n", stderr);
                    success = 0;
                    goto done;
                }
                line[length++] = (char)ch;
            }
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
        if (strcmp(line, ".exit") == 0 || strcmp(line, "exit") == 0 ||
            strcmp(line, "quit") == 0)
            break;
        if (terminal && !save_history(&history, line)) {
            fputs("Error: could not save command history.\n", stderr);
            success = 0;
            break;
        }
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
done:
#ifndef _WIN32
    if (settings_changed && tcsetattr(STDIN_FILENO, TCSANOW,
                                      &original_settings) != 0) {
        fputs("Error: could not restore terminal settings.\n", stderr);
        success = 0;
    }
#endif
    free(line);
    free_history(&history);
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
