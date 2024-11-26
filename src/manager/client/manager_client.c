#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <limits.h>
#include <netinet/in.h>
#include <errno.h>
#include <manager_server.h>
#include <utils.h>
#include <stdint.h>
#include <getopt.h>
#include <args.h>
#include <arpa/inet.h>

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

#define INVALID_ARGS_MSG "Invalid arguments. Use -h for help\n"

#define C_HISTORY_COMMAND_NAME "chistory"
#define C_CURRENT_COMMAND_NAME "ccurrent"
#define C_RECORD_COMMAND_NAME "crecord"
#define BYTES_S_COMMAND_NAME "bytess"
#define BYTES_R_COMMAND_NAME "bytesr"
#define BYTES_T_COMMAND_NAME "bytest"
#define MAX_CON_COMMAND_NAME "cmax"

#define REQUEST_SEND_ERROR_MSG "Error sending request to server.\n"
#define RESPONSE_RECV_ERROR_MSG "Error receiving response from server.\n"
#define PROT_VERSION_ERROR_MSG "Invalid version of protocol.\n"
#define BAD_REQUEST_ERROR_MSG "Bad request.\n"
#define UNAUTHORIZED_ERROR_MSG "Unauthorized.\n"


/** Manager Protocol - Client
 *
 * Usage: 
 * 
 * $> mng -t [TOKEN] -c [COMMAND] -v [VALUE]
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
typedef struct mngargs {
    char * mng_addr;
    unsigned short mng_port;
    char * token;
    char * command;
    size_t config_value;
} mngargs;

static uint8_t get_command_id(char * command);
static void print_data_info(uint8_t command_id, size_t data);
static void parse_args(const int argc, char ** argv, mngargs * args);
static void usage(const char *progname);
static unsigned short value(const char *s);
static unsigned short port(const char *s);

static char * error_msg = NULL;

int main(int argc, char * argv[]) {
    mngargs args;
    parse_args(argc, argv, &args);

    uint8_t request_buffer[REQUEST_LEN];

    request_buffer[VERSION_OFFSET] = VERSION;

    uint8_t command_id = get_command_id(args.command);

    if (command_id == ERROR_CODE) {
        error_msg = INVALID_ARGS_MSG;
        goto finally;
    }

    if (command_id == SET_MAX_CONECTIONS && args.config_value != 0) {
        request_buffer[VALUE_OFFSET] = args.config_value; 
    }

    if (command_id == SET_MAX_CONECTIONS && args.config_value == 0) {
        error_msg = INVALID_ARGS_MSG;
        goto finally;
    }

    request_buffer[COMMAND_OFFSET] = command_id;

    if (strlen(args.token) != TOKEN_LEN) {
        error_msg = UNAUTHORIZED_ERROR_MSG;
        goto finally;
    }

    memcpy(&request_buffer[AUTH_TOKEN_OFFSET], args.token, TOKEN_LEN);

    int server = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); 

    if (server == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(args.mng_port);
    server_addr.sin_addr.s_addr = inet_addr(args.mng_addr);

    socklen_t server_addr_len = sizeof(server_addr);

    ssize_t bytes_send = sendto(server, request_buffer, REQUEST_LEN, 0, (struct sockaddr *) &server_addr, server_addr_len);

    if (bytes_send < 0) {
        error_msg = REQUEST_SEND_ERROR_MSG;
        goto finally;
    }

    uint8_t response_buffer[RESPONSE_LEN];

    size_t bytes_rcv = recvfrom(server, response_buffer, RESPONSE_LEN, 0, (struct sockaddr *) &server_addr, &server_addr_len);

    if (bytes_rcv < 0) {
        error_msg = RESPONSE_RECV_ERROR_MSG;
        goto finally;
    }

    if (response_buffer[VERSION_OFFSET] != VERSION) {
        error_msg = PROT_VERSION_ERROR_MSG;
        goto finally;
    }

    if (response_buffer[STATUS_OFFSET] != OK && response_buffer[STATUS_OFFSET] != UNAUTHORIZED) {
        error_msg = BAD_REQUEST_ERROR_MSG;
        goto finally;
    }

    if (response_buffer[STATUS_OFFSET] == UNAUTHORIZED) {
        error_msg = UNAUTHORIZED_ERROR_MSG; 
        goto finally;
    }

    print_data_info(command_id, response_buffer[DATA_OFFSET]);

finally:
    close(server);

    if (error_msg != NULL) {
        fprintf(stderr, error_msg);
        return 1;
    }

    return 0;
}


// Se que es horrible, pero no se me ocurre mejor manera.
static uint8_t get_command_id(char * command) {
    if (command == NULL) {
        return ERROR_COMMAND; 
    }

    if (strcmp(command, C_CURRENT_COMMAND_NAME) == 0) {
        return CURRENT_CONECTIONS;
    } else if (strcmp(command, C_HISTORY_COMMAND_NAME) == 0) {
        return HIST_CONECTIONS;
    } else if (strcmp(command, C_RECORD_COMMAND_NAME) == 0) {
        return RECORD_CONCURRENT_CONECTIONS;
    } else if (strcmp(command, BYTES_S_COMMAND_NAME) == 0) {
        return BYTES_SEND;
    } else if (strcmp(command, BYTES_R_COMMAND_NAME) == 0) {
        return BYTES_RECEIVED;
    } else if (strcmp(command, BYTES_T_COMMAND_NAME) == 0) {
        return TOTAL_BYTES_TRANSFERED;
    } else if (strcmp(command, MAX_CON_COMMAND_NAME) == 0) {
        return SET_MAX_CONECTIONS;
    }
    return ERROR_COMMAND;
}

static void print_data_info(u_int8_t command_id, size_t data) {
    switch(command_id) {
        case CURRENT_CONECTIONS: fprintf(stdout, "Current conections: %ld.\n", data); break;
        case HIST_CONECTIONS: fprintf(stdout, "Historic conections: %ld.\n", data); break;
        case RECORD_CONCURRENT_CONECTIONS: fprintf(stdout, "Record concurrent conections: %ld.\n", data); break;
        case BYTES_SEND: fprintf(stdout, "Bytes send: %ld.\n", data); break;
        case BYTES_RECEIVED:  fprintf(stdout, "Bytes received: %ld.\n", data); break;
        case TOTAL_BYTES_TRANSFERED: fprintf(stdout, "Bytes transfered: %ld.\n", data); break;
        case SET_MAX_CONECTIONS: fprintf(stdout, "Succes in configuration change.\n"); break;
        default: fprintf(stdout, "Invalid Response."); break;
    }
}

static void parse_args(const int argc, char ** argv, mngargs * args) {
    memset(args, 0, sizeof(*args));

    args->mng_addr = "127.0.0.1";
    args->mng_port = 8086;

    args->token = NULL;
    args->command = NULL;
    args->config_value = 0;

    int c;

    while (true) {
        int option_index = 0;

        c = getopt_long(argc, argv, "L:P:hl:t:c:n:", NULL, &option_index);
        if (c == -1) break;

        switch(c) {
            case 'L':
                args->mng_addr = optarg;
                break;
            case 'P':
                args->mng_port = port(optarg); 
                break;
            case 'h':
                usage(argv[0]);
                break;
            case 't':
                args->token = optarg;
                break;
            case 'c':
                args->command = optarg;
                break;
            case 'n':
                args->config_value = value(optarg);
                break;
            default:
                fprintf(stderr, "unknown argument %d.\n", c);
                exit(1);
        } 
    }

    if (optind < argc) {
        fprintf(stderr, "argument not accepted: ");
        while (optind < argc) {
            fprintf(stderr, "%s ", argv[optind++]);
        }
        fprintf(stderr, "\n");
        exit(1);       
    }
}

static void usage(const char *progname) {
    fprintf(stderr,
        "Usage: %s [OPTION]...\n"
        "\n"
        "   -h               Imprime la ayuda y termina.\n"
        "   -L               Direccion donde sirve el servicio de management.\n"
        "   -P               Puerto entrante de el servicio de management.\n"
        "   -t               Token de autorización.\n"
        "   -c               Comando que se desea ejecutar.\n"
        "   -n               Valor de la configuracion que se desea modificar.\n"
        "\n",
        progname);
    exit(1);
}

static unsigned short
port(const char *s) {
     char *end     = 0;
     const long sl = strtol(s, &end, 10);

     if (end == s|| '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 0 || sl > USHRT_MAX) {
         fprintf(stderr, "port should in in the range of 1-65536: %s\n", s);
         exit(1);
         return 1;
     }
     return (unsigned short)sl;
}

static unsigned short
value(const char *s) {
    char *end = 0;
    const long sl = strtol(s, &end, 10);

    if (end == s || '\0' != *end
        || ((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
        || sl < 1 || sl > 500) {
        fprintf(stderr, "Number must be in the range of 1-500: %s\n", s);
        exit(1);
        return 1;
    }

    return (unsigned short)sl;
}
