#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libvna/vna.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
        return EXIT_FAILURE; \
    } \
} while (0)

int main(int argc, char **argv)
{
    vna_status status;
    vna_connection *connection;
    char sentinel;
    char *response = &sentinel;
    unsigned int port;

    CHECK(vna_connect(NULL, 0, &status) == NULL);
    CHECK(status == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_connect("", 0, &status) == NULL);
    CHECK(status == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_connect("not-an-ip", 0, &status) == NULL);
    CHECK(status == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_connect("127.0.0.1", 65536, &status) == NULL);
    CHECK(status == VNA_STATUS_OUT_OF_RANGE);
    CHECK(vna_query(NULL, "*IDN?", &response) == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(response == &sentinel);
    CHECK(vna_send(NULL, "*CLS") == VNA_STATUS_INVALID_ARGUMENT);
    vna_disconnect(NULL);
    vna_free_response(NULL);
    if (argc == 1)
        return EXIT_SUCCESS;
    CHECK(argc == 4);
    port = (unsigned int)strtoul(argv[2], NULL, 10);
    connection = vna_connect(argv[1], port, &status);
    CHECK(connection != NULL);
    CHECK(status == VNA_STATUS_SUCCESS);
    CHECK(vna_send(connection, "") == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_send(connection, NULL) == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_send(connection, "*CLS\n*RST") == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_send(connection, "*CLS\r") == VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_query(connection, "*IDN?", NULL) == VNA_STATUS_INVALID_ARGUMENT);
    status = vna_query(connection, "*IDN?", &response);
    if (strcmp(argv[3], "normal") == 0) {
        CHECK(status == VNA_STATUS_SUCCESS);
        CHECK(strcmp(response, "Mock,VNA,123,1.0") == 0);
        vna_free_response(response);
        CHECK(vna_send(connection, "*CLS") == VNA_STATUS_SUCCESS);
        CHECK(vna_query(connection, "VALUE?", &response) == VNA_STATUS_SUCCESS);
        CHECK(strcmp(response, "42") == 0);
        vna_free_response(response);
    } else if (strcmp(argv[3], "buffered") == 0) {
        CHECK(status == VNA_STATUS_SUCCESS);
        CHECK(strcmp(response, "first") == 0);
        vna_free_response(response);
        CHECK(vna_query(connection, "VALUE?", &response) == VNA_STATUS_SUCCESS);
        CHECK(strcmp(response, "second") == 0);
        vna_free_response(response);
    } else {
        CHECK(status == (strcmp(argv[3], "timeout") == 0 ?
                         VNA_STATUS_TIMEOUT : VNA_STATUS_IO_ERROR));
        CHECK(response == &sentinel);
        CHECK(vna_query(connection, "VALUE?", &response) == VNA_STATUS_IO_ERROR);
        CHECK(response == &sentinel);
    }
    vna_disconnect(connection);
    return EXIT_SUCCESS;
}
