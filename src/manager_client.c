#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "manager_server.h"
#include "utils.h"

#define NORMAL_AMOUNT_OF_ARGS 3
#define HELP_AMOUNT_OF_ARGS 2

#define HELP_ARG_IDX 1
#define BIN_ARG_IDX 0
#define COMMAND_ARG_IDX 2

#define HELP_FLAG "-h"


#define USAGE_MSG "usage: %s [token] [command]\n\
commads:\n\
chistory - History of connections\n\
ccurrent - Current connections\n\
crecord  - Record of concurrent connections\n\
bytess   - Bytes send\n\
bytesr   - Bytes recieved\n\
bytest   - Bytes total\n"

#define INVALID_ARGS_MSG "Invalid arguments. Use -h for help"

#define C_HISTORY_COMMAND_NAME "chistory"
#define C_CURRENT_COMMAND_NAME "ccurrent"
#define C_RECORD_COMMAND_NAME "crecord"
#define BYTES_S_COMMAND_NAME "bytess"
#define BYTES_R_COMMAND_NAME "bytesr"
#define BYTES_T_COMMAND_NAME "bytest"


/** Manager Protocol - Client
 *
 * Usage: 
 * 
 * $> manager [TOKEN] [COMMAND]
 * 
 * Command:
 * 
 * chistory - History of connections
 * ccurrent - Current connections
 * crecord  - Record of concurrent connections
 * bytess   - Bytes send
 * bytesr   - Bytes recieved
 * bytest   - Bytes total
 * 

*/

static char * error_msg;

int main(int argc, char * argv[]) {
    if (argc = HELP_AMOUNT_OF_ARGS && strcmp(argv[HELP_ARG_IDX], HELP_FLAG)) {
        fprintf(stdout, USAGE_MSG, BIN_ARG_IDX);
        return;
    }

    if (argc != NORMAL_AMOUNT_OF_ARGS) {
        error_msg = INVALID_ARGS_MSG; 
        goto finally;
    }

    uint8_t request_buffer[REQUEST_RESPONSE_LEN];

    request_buffer[VERSION_OFFSET] = VERSION;

    uint8_t command_id = get_command_id(argv[COMMAND_ARG_IDX]);

    if (command_id == ERROR_CODE) {
        error_msg = INVALID_ARGS_MSG;
        goto finally;
    }

    request_buffer[COMMAND_OFFSET] = command_id;

    memcpy(&request_buffer[AUTH_TOKEN_OFFSET], argv[1], TOKEN_LEN);

    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_UDP); 

    if (server == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MANAGER_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    socklen_t server_addr_len = sizeof(server_addr);

    ssize_t bytes_send = sendto(server, request_buffer, REQUEST_RESPONSE_LEN, 0, (struct sockaddr *) &server_addr, server_addr_len);

    if (bytes_send < 0) {
        error_msg = "Error sending request to server.";
        goto finally;
    }

    uint8_t response_buffer[REQUEST_RESPONSE_LEN];

    size_t bytes_rcv = recvfrom(server, response_buffer, REQUEST_RESPONSE_LEN, 0, (struct sockaddr *) &server_addr, &server_addr_len);

    if (bytes_rcv < 0) {
        error_msg = "Error receiving response from server.";
        goto finally;
    }

    if (response_buffer[VERSION_OFFSET] != VERSION) {
        error_msg = "kdowkdow";
        goto finally;
    }

    if (response_buffer[STATUS_OFFSET] != OK || response_buffer[STATUS_OFFSET] != UNAUTHORIZED) {
        error_msg = "jfowejfoew";
        goto finally;
    }

    if (response_buffer[STATUS_OFFSET] == UNAUTHORIZED) {
        error_msg = "dkwodkwo";
        goto finally;
    }

    print_data_info(command_id, response_buffer[DATA_OFFSET]);

finally:
    close(server);
    return 0;
}


// Se que es horrible, pero no se me ocurre rapido mejor manera.
uint8_t get_command_id(char * command) {
    if (strcmp(command, C_CURRENT_COMMAND_NAME)) {
        return CURRENT_CONECTIONS;
    } else if (strcmp(command, C_HISTORY_COMMAND_NAME)) {
        return HIST_CONECTIONS;
    } else if (strcmp(command, C_RECORD_COMMAND_NAME)) {
        return RECORD_CONCURRENT_CONECTIONS;
    } else if (strcmp(command, BYTES_S_COMMAND_NAME)) {
        return BYTES_SEND;
    } else if (strcmp(command, BYTES_R_COMMAND_NAME)) {
        return BYTES_RECEIVED;
    } else if (strcmp(command, BYTES_T_COMMAND_NAME)) {
        return TOTAL_BYTES_TRANSFERED;
    }
    return -1;
}

void print_data_info(u_int8_t command_id, size_t data) {
    switch(command_id) {
        case CURRENT_CONECTIONS: fprintf(stdout, "Current conections: %d\n", data); break;
        case HIST_CONECTIONS: fprintf(stdout, "Historic conections: %d\n", data); break;
        case RECORD_CONCURRENT_CONECTIONS: fprintf(stdout, "Record concurrent conections: %d\n", data); break;
        case BYTES_SEND: fprintf(stdout, "Bytes send: %d\n", data); break;
        case BYTES_RECEIVED:  fprintf(stdout, "Bytes received: %d\n", data); break;
        case TOTAL_BYTES_TRANSFERED: fprintf(stdout, "Bytes transfered: %d\n", data);  break;
        default: fprintf(stdout, "Invalid Response."); break;
    }
}