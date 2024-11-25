#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <selector.h>
#include <pop3.h>
#include <utils.h>
#include <manager_server.h>

#define BACKLOG 20
#define POP3_DEFAULT_PORT 8085
#define MANAGER_DEFAULT_PORT 8086 

int pop3_passive_socket;
int manager_socket;
char * error_msg;

int main() {
    pop3_passive_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (pop3_passive_socket == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    }

    setsockopt(pop3_passive_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    struct sockaddr_in my_listen_addr;
    memset(&my_listen_addr, 0, sizeof(my_listen_addr));
    my_listen_addr.sin_family = AF_INET;
    my_listen_addr.sin_port = htons(POP3_DEFAULT_PORT);
    my_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(pop3_passive_socket, (struct sockaddr *) &my_listen_addr, sizeof(my_listen_addr)) == ERROR_CODE) {
        error_msg = SOCKET_BINDING_ERROR_MSG;
        goto finally;
    }

    if (listen(pop3_passive_socket, BACKLOG) == ERROR_CODE) {
        error_msg = SOCKET_LISTENING_ERROR_MSG;
        goto finally;
    }

    if (selector_fd_set_nio(pop3_passive_socket) == ERROR_CODE) {
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

    ss = selector_register(selector, pop3_passive_socket, &pop3, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        error_msg = SELECTOR_REGISTER_ERROR_MSG;
        goto finally;
    }

    manager_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (manager_socket == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    } 

    setsockopt(manager_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    struct sockaddr_in manager_listen_addr;
    memset(&manager_listen_addr, 0, sizeof(manager_listen_addr));
    manager_listen_addr.sin_family = AF_INET;
    manager_listen_addr.sin_port = htons(MANAGER_PORT);
    manager_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(manager_socket, (struct sockaddr *) &manager_listen_addr, sizeof(manager_listen_addr)) == ERROR_CODE) {
        error_msg = SOCKET_BINDING_ERROR_MSG;
        goto finally;
    }

    if (selector_fd_set_nio(manager_socket) == ERROR_CODE) {
        error_msg = SELECTOR_SETTING_PASSIVE_SOCKET_NIO_ERROR_MSG;
        goto finally;
    }

    const struct fd_handler manager = {
        .handle_read = manager_handle_connection,
        .handle_write = NULL,
        .handle_close = NULL,
    };

    ss = selector_register(selector, manager_socket, &manager, OP_READ, NULL);

    if (ss != SELECTOR_SUCCESS) {
        error_msg = SELECTOR_REGISTER_ERROR_MSG;
        goto finally;
    }

    while (1) {
        error_msg = NULL;
        ss = selector_select(selector);
        if (ss != SELECTOR_SUCCESS) {
            error_msg = SELECTOR_SELECT_ERROR_MSG;
            goto finally;
        }
    }

finally:
    close(pop3_passive_socket);
    close(manager_socket);

    selector_close();
    
    fprintf(stderr, "%s\n", error_msg);
    return 0;
}

