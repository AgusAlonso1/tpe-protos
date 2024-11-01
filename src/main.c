#include <sys/socket.h>
#include "utils.h"

#define ERROR_CODE -1
#define DEFAULT_PROTOCOL 0



int passive_socket;

int main() {
    passive_socket = socket(AF_INET | AF_INET6, SOCK_NONBLOCK, DEFAULT_PROTOCOL); // QUE FLAGS PONER ACA ???
    validate(passive_socket, SOCKET_CREATION_ERROR_MSG);

    struct sockaddr my_listen_addr;
    // NOSE BIEN COMO INICIALIZAR ESTE STRUCT, QUE TIPO DE DATO USAR, LA DIFERENCIA ENTRE SOCKADDR Y SOCKADDR_IN
    validate(bind(passive_socket, &my_listen_addr, sizeof(my_listen_addr)), SOCKET_BINDING_ERROR_MSG);

    validate(listen(passive_socket, 20), SOCKET_LISTENING_ERROR_MSG);
    //20 ESTA BIEN EN ESTE FLAG??
    
    //COMO SEGUIR?

    return 0;
}

