#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include "selector.h"
#include "pop3.h"
#include "utils.h"

#define ERROR_CODE -1
#define BACKLOG 20
#define DEFAULT_PORT 8085 

int passive_socket;
char * error_msg;

int main() {
    passive_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (passive_socket == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    }

    setsockopt(passive_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    struct sockaddr_in my_listen_addr;
    memset(&my_listen_addr, 0, sizeof(my_listen_addr));
    my_listen_addr.sin_family = AF_INET;
    my_listen_addr.sin_port = htons(DEFAULT_PORT);
    my_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(passive_socket, (struct sockaddr *) &my_listen_addr, sizeof(my_listen_addr)) == ERROR_CODE) {
        error_msg = SOCKET_BINDING_ERROR_MSG;
        goto finally;
    }

    if (listen(passive_socket, BACKLOG) == ERROR_CODE) {
        error_msg = SOCKET_LISTENING_ERROR_MSG;
        goto finally;
    }

    if (selector_fd_set_nio(passive_socket) == ERROR_CODE) {
        error_msg = SELECTOR_SETTING_PASSIVE_SOCKET_NIO_ERROR_MSG;
        goto finally;
    }

    const struct selector_init selector_config = {
        .signal = SIGALRM,
        .select_timeout = {
            .tv_sec = 10,
            .tv_nsec = 0,
        },
    };

    if (selector_init(&selector_config) == ERROR_CODE) {
        error_msg = SELECTOR_INIT_ERROR_MSG;
        goto finally;
    }

    fd_selector selector = NULL; 

    selector_status ss = SELECTOR_SUCCESS; 
    selector = selector_new(1024);
    

    if (selector ==  NULL) {
        error_msg = SELECTOR_INIT_ERROR_MSG;
        goto finally;
    }

    const struct fd_handler pop3 = {
        .handle_read = pop3_passive_accept,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    ss = selector_register(selector, passive_socket, &pop3, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        error_msg = SELECTOR_REGISTER_ERROR_MSG;
        goto finally;
    }

    while (1) {
        error_msg = NULL;
        ss = selector_select(selector);
        if (ss != SELECTOR_SUCCESS) {
            error_msg = "d;wd";
            goto finally;
        }
    }

finally:
    close(passive_socket);

    selector_close();
    
    fprintf(stderr, "%s\n", error_msg);
    return 0;
}

