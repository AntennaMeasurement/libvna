#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libvnascpi/vna.h"

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
    unsigned char *data = (unsigned char *)&sentinel;
    size_t data_length = 123;
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
    CHECK(vna_query_block(NULL, "DATA?", &data, &data_length) ==
          VNA_STATUS_INVALID_ARGUMENT);
    CHECK(data == (unsigned char *)&sentinel);
    CHECK(data_length == 123);
    CHECK(vna_query_block(NULL, "DATA?", NULL, &data_length) ==
          VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_query_block(NULL, "DATA?", &data, NULL) ==
          VNA_STATUS_INVALID_ARGUMENT);
    CHECK(vna_send(NULL, "*CLS") == VNA_STATUS_INVALID_ARGUMENT);
    vna_disconnect(NULL);
    vna_free_response(NULL);
    vna_free_data(NULL);
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
    } else if (strcmp(argv[3], "block") == 0) {
        static const unsigned char expected[] = { 0, '\n', 0xff, 'a', 'b', 'c' };
        CHECK(status == VNA_STATUS_SUCCESS);
        vna_free_response(response);
        CHECK(vna_query_block(connection, "MMEM:DATA? \"C:\\screen.png\"",
                              &data, &data_length) == VNA_STATUS_SUCCESS);
        CHECK(data_length == sizeof(expected));
        CHECK(memcmp(data, expected, sizeof(expected)) == 0);
        vna_free_data(data);
        CHECK(vna_query(connection, "VALUE?", &response) == VNA_STATUS_SUCCESS);
        CHECK(strcmp(response, "42") == 0);
        vna_free_response(response);
    } else if (strcmp(argv[3], "invalid-block") == 0) {
        CHECK(status == VNA_STATUS_SUCCESS);
        vna_free_response(response);
        CHECK(vna_query_block(connection, "DATA?", &data, &data_length) ==
              VNA_STATUS_IO_ERROR);
        CHECK(data == (unsigned char *)&sentinel);
        CHECK(data_length == 123);
        CHECK(vna_query(connection, "VALUE?", &response) == VNA_STATUS_IO_ERROR);
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
