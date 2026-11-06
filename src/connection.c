#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
#define _POSIX_C_SOURCE 200809L
#endif

#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <uv.h>

#include "libvnascpi/connection.h"

struct vna_connection {
    uv_loop_t loop;
    uv_tcp_t tcp;
    uv_timer_t timer;
    int done;
    int failed;
    vna_status status;
    char *buffer;
    size_t length;
    size_t capacity;
    size_t read_target;
    int read_to_newline;
};

const char *vna_status_string(vna_status status)
{
    switch (status) {
    case VNA_STATUS_SUCCESS: return "Success";
    case VNA_STATUS_INVALID_ARGUMENT: return "Invalid argument";
    case VNA_STATUS_OUT_OF_RANGE: return "Value out of range";
    case VNA_STATUS_NO_MEMORY: return "Out of memory";
    case VNA_STATUS_CONNECTION_ERROR: return "Connection failed";
    case VNA_STATUS_TIMEOUT: return "Operation timed out";
    case VNA_STATUS_IO_ERROR: return "Connection closed or I/O failed";
    default: return "Unknown status";
    }
}

static void finish(vna_connection *connection, vna_status status)
{
    connection->status = status;
    connection->done = 1;
    uv_timer_stop(&connection->timer);
    uv_read_stop((uv_stream_t *)&connection->tcp);
    if (status != VNA_STATUS_SUCCESS) {
        connection->failed = 1;
        if (!uv_is_closing((uv_handle_t *)&connection->tcp))
            uv_close((uv_handle_t *)&connection->tcp, NULL);
    }
}

static void timed_out(uv_timer_t *timer)
{
    finish(timer->data, VNA_STATUS_TIMEOUT);
}

static void begin(vna_connection *connection)
{
    connection->done = 0;
    connection->status = VNA_STATUS_SUCCESS;
    uv_timer_start(&connection->timer, timed_out, VNA_TIMEOUT_MS, 0);
}

static vna_status wait_for_operation(vna_connection *connection)
{
    while (!connection->done)
        uv_run(&connection->loop, UV_RUN_ONCE);
    /* A timed-out connect/write request must finish before its stack unwinds. */
    if (connection->failed)
        uv_run(&connection->loop, UV_RUN_DEFAULT);
    return connection->status;
}

static void connected(uv_connect_t *request, int status)
{
    vna_connection *connection = request->data;
    if (!connection->done)
        finish(connection, status == 0 ? VNA_STATUS_SUCCESS
                                      : VNA_STATUS_CONNECTION_ERROR);
}

vna_connection *vna_connect(const char *ip, unsigned int port, vna_status *status)
{
    struct sockaddr_storage address;
    vna_connection *connection = NULL;
    uv_connect_t request;
    vna_status result = VNA_STATUS_INVALID_ARGUMENT;

    if (ip == NULL || ip[0] == '\0')
        goto exit;
    if (port > 65535) {
        result = VNA_STATUS_OUT_OF_RANGE;
        goto exit;
    }
    if (port == 0)
        port = VNA_DEFAULT_PORT;
    if (uv_ip4_addr(ip, (int)port, (struct sockaddr_in *)&address) != 0 &&
        uv_ip6_addr(ip, (int)port, (struct sockaddr_in6 *)&address) != 0)
        goto exit;

    result = VNA_STATUS_NO_MEMORY;
    connection = calloc(1, sizeof(*connection));
    if (connection == NULL)
        goto exit;
    if (uv_loop_init(&connection->loop) != 0) {
        free(connection);
        connection = NULL;
        goto exit;
    }
    result = VNA_STATUS_CONNECTION_ERROR;
    if (uv_tcp_init(&connection->loop, &connection->tcp) != 0) {
        uv_loop_close(&connection->loop);
        free(connection);
        connection = NULL;
        goto exit;
    }
    if (uv_timer_init(&connection->loop, &connection->timer) != 0) {
        uv_close((uv_handle_t *)&connection->tcp, NULL);
        uv_run(&connection->loop, UV_RUN_DEFAULT);
        uv_loop_close(&connection->loop);
        free(connection);
        connection = NULL;
        goto exit;
    }
    connection->tcp.data = connection;
    connection->timer.data = connection;
    request.data = connection;
    begin(connection);
    if (uv_tcp_connect(&request, &connection->tcp,
                       (const struct sockaddr *)&address, connected) != 0)
        finish(connection, VNA_STATUS_CONNECTION_ERROR);
    result = wait_for_operation(connection);
    if (result != VNA_STATUS_SUCCESS) {
        vna_disconnect(connection);
        connection = NULL;
    }
exit:
    if (status != NULL)
        *status = result;
    return connection;
}

void vna_disconnect(vna_connection *connection)
{
    if (connection == NULL)
        return;
    if (!uv_is_closing((uv_handle_t *)&connection->tcp))
        uv_close((uv_handle_t *)&connection->tcp, NULL);
    uv_close((uv_handle_t *)&connection->timer, NULL);
    uv_run(&connection->loop, UV_RUN_DEFAULT);
    uv_loop_close(&connection->loop);
    free(connection->buffer);
    free(connection);
}

static void written(uv_write_t *request, int status)
{
    vna_connection *connection = request->data;
    if (!connection->done)
        finish(connection, status == 0 ? VNA_STATUS_SUCCESS : VNA_STATUS_IO_ERROR);
}

vna_status vna_send(vna_connection *connection, const char *command)
{
    size_t length;
    char *line;
    uv_buf_t buffer;
    uv_write_t request;
    vna_status status;

    if (connection == NULL || command == NULL || command[0] == '\0' ||
        strpbrk(command, "\r\n") != NULL)
        return VNA_STATUS_INVALID_ARGUMENT;
    if (connection->failed)
        return VNA_STATUS_IO_ERROR;
    length = strlen(command);
    if (length >= UINT_MAX)
        return VNA_STATUS_OUT_OF_RANGE;
    line = malloc(length + 1);
    if (line == NULL)
        return VNA_STATUS_NO_MEMORY;
    memcpy(line, command, length);
    line[length] = '\n';
    buffer = uv_buf_init(line, (unsigned int)(length + 1));
    request.data = connection;
    begin(connection);
    if (uv_write(&request, (uv_stream_t *)&connection->tcp, &buffer, 1, written) != 0)
        finish(connection, VNA_STATUS_IO_ERROR);
    status = wait_for_operation(connection);
    free(line);
    return status;
}

static void allocate_read(uv_handle_t *handle, size_t suggested, uv_buf_t *buffer)
{
    (void)handle;
    (void)suggested;
    buffer->base = malloc(4096);
    buffer->len = buffer->base == NULL ? 0 : 4096;
}

static void received(uv_stream_t *stream, ssize_t count, const uv_buf_t *buffer)
{
    vna_connection *connection = stream->data;
    if (count < 0) {
        finish(connection, count == UV_ENOBUFS ? VNA_STATUS_NO_MEMORY
                                               : VNA_STATUS_IO_ERROR);
    } else if (count > 0) {
        size_t needed;
        if ((size_t)count > SIZE_MAX - connection->length) {
            finish(connection, VNA_STATUS_OUT_OF_RANGE);
            free(buffer->base);
            return;
        }
        needed = connection->length + (size_t)count;
        if (needed > connection->capacity) {
            size_t capacity = needed <= SIZE_MAX / 2 ? needed * 2 : needed;
            char *grown = realloc(connection->buffer, capacity);
            if (grown == NULL) {
                finish(connection, VNA_STATUS_NO_MEMORY);
                free(buffer->base);
                return;
            }
            connection->buffer = grown;
            connection->capacity = capacity;
        }
        memcpy(connection->buffer + connection->length, buffer->base, (size_t)count);
        connection->length = needed;
        if ((connection->read_to_newline &&
             memchr(connection->buffer, '\n', connection->length) != NULL) ||
            (!connection->read_to_newline &&
             connection->length >= connection->read_target))
            finish(connection, VNA_STATUS_SUCCESS);
    }
    free(buffer->base);
}

static vna_status read_response(vna_connection *connection, size_t target,
                                int to_newline)
{
    int ready = to_newline ?
        memchr(connection->buffer, '\n', connection->length) != NULL :
        connection->length >= target;
    if (ready)
        return VNA_STATUS_SUCCESS;

    connection->read_target = target;
    connection->read_to_newline = to_newline;
    begin(connection);
    if (uv_read_start((uv_stream_t *)&connection->tcp,
                      allocate_read, received) != 0)
        finish(connection, VNA_STATUS_IO_ERROR);
    return wait_for_operation(connection);
}

vna_status vna_query(vna_connection *connection, const char *command, char **response)
{
    vna_status status;
    char *newline;
    char *text;
    size_t length;
    size_t consumed;

    if (response == NULL)
        return VNA_STATUS_INVALID_ARGUMENT;
    status = vna_send(connection, command);
    if (status != VNA_STATUS_SUCCESS)
        return status;
    newline = connection->length == 0 ? NULL :
        memchr(connection->buffer, '\n', connection->length);
    if (newline == NULL) {
        status = read_response(connection, 0, 1);
        if (status != VNA_STATUS_SUCCESS)
            return status;
        newline = memchr(connection->buffer, '\n', connection->length);
    }
    length = (size_t)(newline - connection->buffer);
    consumed = length + 1;
    if (length > 0 && connection->buffer[length - 1] == '\r')
        --length;
    text = malloc(length + 1);
    if (text == NULL) {
        finish(connection, VNA_STATUS_NO_MEMORY);
        return VNA_STATUS_NO_MEMORY;
    }
    memcpy(text, connection->buffer, length);
    text[length] = '\0';
    connection->length -= consumed;
    memmove(connection->buffer, connection->buffer + consumed, connection->length);
    *response = text;
    return VNA_STATUS_SUCCESS;
}

vna_status vna_query_block(vna_connection *connection, const char *command,
                           unsigned char **data, size_t *length)
{
    vna_status status;
    size_t header_length;
    size_t payload_length = 0;
    size_t payload_end;
    size_t consumed;
    unsigned int length_digits;
    unsigned char *payload;

    if (data == NULL || length == NULL)
        return VNA_STATUS_INVALID_ARGUMENT;
    status = vna_send(connection, command);
    if (status != VNA_STATUS_SUCCESS)
        return status;

    status = read_response(connection, 2, 0);
    if (status != VNA_STATUS_SUCCESS)
        return status;
    if (connection->buffer[0] != '#' ||
        connection->buffer[1] < '1' || connection->buffer[1] > '9') {
        finish(connection, VNA_STATUS_IO_ERROR);
        return VNA_STATUS_IO_ERROR;
    }
    length_digits = (unsigned int)(connection->buffer[1] - '0');
    header_length = (size_t)length_digits + 2;
    status = read_response(connection, header_length, 0);
    if (status != VNA_STATUS_SUCCESS)
        return status;
    for (size_t i = 2; i < header_length; ++i) {
        unsigned int digit;
        if (connection->buffer[i] < '0' || connection->buffer[i] > '9') {
            finish(connection, VNA_STATUS_IO_ERROR);
            return VNA_STATUS_IO_ERROR;
        }
        digit = (unsigned int)(connection->buffer[i] - '0');
        if (payload_length > (SIZE_MAX - digit) / 10) {
            finish(connection, VNA_STATUS_OUT_OF_RANGE);
            return VNA_STATUS_OUT_OF_RANGE;
        }
        payload_length = payload_length * 10 + digit;
    }
    if (payload_length > SIZE_MAX - header_length) {
        finish(connection, VNA_STATUS_OUT_OF_RANGE);
        return VNA_STATUS_OUT_OF_RANGE;
    }
    payload_end = header_length + payload_length;
    if (payload_end == SIZE_MAX) {
        finish(connection, VNA_STATUS_OUT_OF_RANGE);
        return VNA_STATUS_OUT_OF_RANGE;
    }
    status = read_response(connection, payload_end + 1, 0);
    if (status != VNA_STATUS_SUCCESS)
        return status;
    if (connection->buffer[payload_end] == '\r') {
        if (payload_end > SIZE_MAX - 2) {
            finish(connection, VNA_STATUS_OUT_OF_RANGE);
            return VNA_STATUS_OUT_OF_RANGE;
        }
        status = read_response(connection, payload_end + 2, 0);
        if (status != VNA_STATUS_SUCCESS)
            return status;
        if (connection->buffer[payload_end + 1] != '\n') {
            finish(connection, VNA_STATUS_IO_ERROR);
            return VNA_STATUS_IO_ERROR;
        }
        consumed = payload_end + 2;
    } else if (connection->buffer[payload_end] == '\n') {
        consumed = payload_end + 1;
    } else {
        finish(connection, VNA_STATUS_IO_ERROR);
        return VNA_STATUS_IO_ERROR;
    }

    payload = payload_length == 0 ? NULL : malloc(payload_length);
    if (payload_length != 0 && payload == NULL) {
        finish(connection, VNA_STATUS_NO_MEMORY);
        return VNA_STATUS_NO_MEMORY;
    }
    if (payload_length != 0)
        memcpy(payload, connection->buffer + header_length, payload_length);
    connection->length -= consumed;
    memmove(connection->buffer, connection->buffer + consumed, connection->length);
    *data = payload;
    *length = payload_length;
    return VNA_STATUS_SUCCESS;
}

void vna_free_response(char *response)
{
    free(response);
}

void vna_free_data(void *data)
{
    free(data);
}
