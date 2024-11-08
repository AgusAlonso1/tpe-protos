#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include "pop3.h"
#include "selector.h"
#include "utils.h"

char buffer[2024];

void pop3_passive_accept(struct selector_key * sk) {
    struct sockaddr_storage active_socket_addr;
    socklen_t active_socket_addr_len = sizeof(active_socket_addr);

    int active_socket_fd = accept(sk->fd, (struct sockaddr *) &active_socket_addr, &active_socket_addr_len);
    if (active_socket_fd == ERROR_CODE) {
        goto fail;
    }

    if (selector_fd_set_nio(active_socket_fd) == ERROR_CODE) {
        goto fail;
    }

    const struct fd_handler pop3_handler = {
        .handle_read = pop3_read,
        .handle_write = pop3_write,
        .handle_close = pop3_close,
        .handle_block = pop3_block,
    };

    if (selector_register(sk->s, active_socket_fd, &pop3_handler, OP_READ & OP_WRITE, NULL) != SELECTOR_SUCCESS) {
        goto fail;
    }
    return;

fail:
    if (active_socket_fd != ERROR_CODE) {
        close(active_socket_fd);
    } 
}

void pop3_write(struct selector_key * sk) {
    printf("%s", buffer); 
}

void pop3_read(struct selector_key * sk) {
    read(sk->fd, buffer, sizeof(buffer));
    buffer[2023] = '\0';
}

void pop3_close(struct selector_key * sk) {

}

void pop3_block(struct selector_key * sk) {

}
