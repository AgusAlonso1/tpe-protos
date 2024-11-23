#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <manager_server.h>
#include <server_info.h>
#include <selector.h>

#define LAST_COMMAND_IDX 5
#define TOKEN "aaaaaaaa"

static char * error_msg;
static ssize_t bytes_sent;

void manager_handle_connection(struct selector_key * sk) {
    struct sockaddr_storage active_socket_addr;
    socklen_t active_socket_addr_len = sizeof(active_socket_addr);

    u_int8_t request_buffer[REQUEST_RESPONSE_LEN];

    ssize_t bytes_rcv = recvfrom(sk->fd, request_buffer, REQUEST_RESPONSE_LEN, MSG_DONTWAIT, (struct sockaddr *) &active_socket_addr, &active_socket_addr_len);

    bytes_received_update(bytes_rcv);

    u_int8_t response_buffer[REQUEST_RESPONSE_LEN];

    response_buffer[VERSION_OFFSET] = VERSION;

    if (bytes_rcv != REQUEST_RESPONSE_LEN) {
        response_buffer[STATUS_OFFSET] = BAD_REQUEST;
        goto send_response;
    }

    if (request_buffer[VERSION_OFFSET] != VERSION) {
        response_buffer[STATUS_OFFSET] = INVALID_VERSION;
        goto send_response;
    }

    u_int8_t command = request_buffer[COMMAND_OFFSET];
    if (command > LAST_COMMAND_IDX) {
        response_buffer[STATUS_OFFSET] = INVALID_COMMAND;
        goto send_response;
    }

    u_int8_t * auth_token[TOKEN_LEN];
    memcpy(auth_token, TOKEN, TOKEN_LEN); 

    if (!strncmp((char *) auth_token, (char *) &(request_buffer[AUTH_TOKEN_OFFSET]), TOKEN_LEN)) {
        response_buffer[STATUS_OFFSET] = UNAUTHORIZED;
        goto send_response;
    }

    size_t data;
    switch (request_buffer[COMMAND_OFFSET]) {
        case HIST_CONECTIONS: data = get_hist_conections(); break;
        case CURRENT_CONECTIONS: data = get_current_conections(); break;
        case BYTES_SEND: data = get_bytes_sent(); break; 
        case BYTES_RECEIVED: data = get_bytes_received(); break;
        case RECORD_CONCURRENT_CONECTIONS: data = get_record_concurrent_conections(); break;
        case TOTAL_BYTES_TRANSFERED: data = get_total_bytes_transfered(); break;
        default: response_buffer[STATUS_OFFSET] = INVALID_COMMAND; break;
    }
    memcpy(&response_buffer[DATA_OFFSET], &data, sizeof(size_t));

send_response:
    bytes_sent = sendto(sk->fd, response_buffer, REQUEST_RESPONSE_LEN, 0, (struct sockaddr *) &active_socket_addr, active_socket_addr_len);

    if (bytes_sent < 0) {
        error_msg = "jdwojdkwodw";
    }

    bytes_sent_update(bytes_sent);

    fprintf(stderr, "%s\n", error_msg);
} 
