#include <string.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <selector.h>
#include <pop3.h>
#include <utils.h>
#include <server_info.h>
#include <manager_server.h>
#include <args.h>

#define BACKLOG 20
#define POP3_DEFAULT_PORT 8085
#define MANAGER_DEFAULT_PORT 8086 

int pop3_passive_socket;
int manager_socket;
char * error_msg;

int main(int argc, char ** argv) {
    init_server_args(argc, argv);
    init_server_info();
    
    pop3_passive_socket = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    if (pop3_passive_socket == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    }

    int dual_stack = 0;

    setsockopt(pop3_passive_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));

    // Permitir conexiones tanto IPV4 como IPV6
    setsockopt(pop3_passive_socket, IPPROTO_IPV6, IPV6_V6ONLY, &dual_stack, sizeof(dual_stack));

    struct sockaddr_in6 my_listen_addr;
    memset(&my_listen_addr, 0, sizeof(my_listen_addr));
    my_listen_addr.sin6_family = AF_INET6;
    my_listen_addr.sin6_port = htons(get_pop3_port());
    inet_pton(AF_INET6, get_pop3_addr(), &my_listen_addr.sin6_addr);

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

    manager_socket = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);

    if (manager_socket == ERROR_CODE) {
        error_msg = SOCKET_CREATION_ERROR_MSG;
        goto finally;
    } 

    setsockopt(manager_socket, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
    setsockopt(manager_socket, IPPROTO_IPV6, IPV6_V6ONLY, &dual_stack, sizeof(dual_stack));

    struct sockaddr_in6 manager_listen_addr;
    memset(&manager_listen_addr, 0, sizeof(manager_listen_addr));
    manager_listen_addr.sin6_family = AF_INET6;
    manager_listen_addr.sin6_port = htons(get_manager_port());
    inet_pton(AF_INET6, get_pop3_addr(), &my_listen_addr.sin6_addr);

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

