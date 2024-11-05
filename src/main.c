#include <string.h>
#include <sys/socket.h>
#include "utils.h"
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>

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

    struct sockaddr_in my_listen_addr;
    memset(&my_listen_addr, 0, sizeof(my_listen_addr));
    my_listen_addr.sin_family = AF_INET;
    my_listen_addr.sin_port = htons(DEFAULT_PORT);
    my_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(passive_socket, (struct sockaddr *) &my_listen_addr, sizeof(my_listen_addr))) {
        error_msg = SOCKET_BINDING_ERROR_MSG;
        goto finally;
    }

    if (listen(passive_socket, BACKLOG)) {
        error_msg = SOCKET_LISTENING_ERROR_MSG;
        goto finally;
    }

    struct sockaddr_in other_socket_addr;
    memset(&other_socket_addr, 0, sizeof(other_socket_addr));
    other_socket_addr.sin_family = AF_INET;
    other_socket_addr.sin_port = htons(8086);
    other_socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
    int addr_len = sizeof(other_socket_addr);
    int fd2 = accept(passive_socket, (struct sockaddr *) &other_socket_addr, (socklen_t *) &addr_len);

    char buffer[220];

    read(fd2, buffer, 220);
    buffer[219] = '\0';

    printf("%s", buffer);

finally:
    close(passive_socket);
    
    fprintf(stderr, "%s\n", error_msg);
    return 0;
}

