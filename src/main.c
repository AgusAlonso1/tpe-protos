#include <sys/socket.h>
#include "utils.h"

#define ERROR_CODE -1
#define DEFAULT_PROTOCOL 0
#define SOCKET_CREATION_ERROR_MSG "Error in creation of socket."


int passive_socket;

int main() {
    passive_socket = socket(AF_INET | AF_INET6, SOCK_NONBLOCK, DEFAULT_PROTOCOL); // QUE FLAGS PONER ACA ???
    validate(passive_socket, SOCKET_CREATION_ERROR_MSG);

    struct sockaddr my_listen_addr;
    // NOSE BIEN COMO INICIALIZAR ESTE STRUCT
    validate(bind(passive_socket, &my_listen_addr, sizeof(my_listen_addr)), SOCKET_BINDING_ERROR_MSG);

    


    return 0;
}

