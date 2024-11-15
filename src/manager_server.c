#include <sys/socket.h>
#include <string.h>
#include "manager_server.h"

#define VERSION 0x0
#define REQUEST_RESPONSE_LEN 10
#define LAST_COMMAND_IDX 5
#define TOKEN 0x1
#define TOKEN_LEN 8

#define VERSION_OFFSET 0

#define AUTH_TOKEN_OFFSET 1
#define COMMAND_OFFSET 9

#define STATUS_OFFSET 1
#define DATA_OFFSET 2

#include "selector.h"
void manager_passive_accept(struct selector_key * sk) {
    struct sockaddr_storage active_socket_addr;
    socklen_t active_socket_addr_len = sizeof(active_socket_addr);

    u_int8_t request_buffer[REQUEST_RESPONSE_LEN];

    ssize_t bytes_rcv = recvfrom(sk->fd, request_buffer, REQUEST_RESPONSE_LEN, MSG_DONTWAIT, (struct sockaddr *) &active_socket_addr, &active_socket_addr_len);

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

    if (!strncmp(auth_token, request_buffer[AUTH_TOKEN_OFFSET], TOKEN_LEN)) {
        response_buffer[STATUS_OFFSET] = UNAUTHORIZED;
        goto send_response;
    }

    size_t data;
    switch (request_buffer[COMMAND_OFFSET]) {
        case HIST_CONECTIONS: break;
        case CURRENT_CONECTIONS: break;
        case BYTES_SEND: break; 
        case BYTES_RECEIVED: break;
        case RECORD_CONCURRENT_CONECTIONS: break;
        case TOTAL_BYTES_TRANSFERED: break;
        default: response_buffer[STATUS_OFFSET] = INVALID_COMMAND; break;
    }
    memcpy(&response_buffer[DATA_OFFSET], &data, sizeof(size_t));

send_response:
    ssize_t bytes_send = sendto(sk->fd, response_buffer, REQUEST_RESPONSE_LEN, 0, (struct sockaddr *) &active_socket_addr, active_socket_addr_len);

    if (bytes_send < 0) {
        
    }
} 
