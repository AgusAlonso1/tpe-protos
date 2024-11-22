#ifndef POP_3
#define POP_3
#include "selector.h"
#include "pop3_parser.h"

#define BUFFER_SIZE 2048
#define ERROR_CODE (-1)
#define USER "USER"
#define PASS "PASS"
#define QUIT "QUIT"
#define STAT "STAT"
#define LIST "LIST"
#define RETR "RETR"
#define DELE "DELE"
#define NOOP "NOOP"
#define RSET "RSET"

enum pop3_state {
    /**
     * Escribe el mensaje de bienvenida
     *
     * Intereses:
     *    - OP_WRITE sobre client_fd
     *
     * Transiciones:
     *    - WAITING_USER cuando se completa el mensaje
     *    - ERROR        ante cualquier error
     */
    WELCOME,
    /**
     * Espera que el cliente ingrese su usuario
     *
     * Intereses:
     *    - OP_READ sobre client_fd
     *
     * Transiciones:
     *    - WAITING_PASS cuando se completa el mensaje
     *    - ERROR        ante cualquier error
     */
    WAITING_USER,
    /**
     * Espera que el cliente ingrese su contraseña
     *
     * Intereses:
     *    - OP_READ sobre client_fd
     *
     * Transiciones:
     *    - AUTH cuando se completa el mensaje
     *    - WAITING_USER si se ingresa nuevamente un usuario
     *    - ERROR        ante cualquier error
     */
    WAITING_PASS,
    /**
     * Espera que el cliente realice pedidos
     *
     * Intereses:
     *    - OP_READ sobre client_fd
     *    - OP_WRITE sobre client_fd
     *
     * Transiciones:
     *    - ERROR        ante cualquier error
     */
    AUTH,
    DONE,
    ERROR,
};

struct pop3_session_data {
    /** Información del cliente **/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    char * username;
    char * password;

    /** Maquina de estados **/
    struct state_machine stm;

    /** Buffers **/
    uint8_t * buff_read;
    uint8_t * buff_write;

    buffer buffer_read;
    buffer buffer_write;

    /** Parser **/
    struct pop3_command_parser parser;

    /** Flag para saber que hacer en el response **/
    bool OK;
    enum pop3_state next_state;

};

static struct fd_handler pop3_handler = {
        .handle_read = pop3_read,
        .handle_write = pop3_write,
        .handle_close = pop3_close,
};


void pop3_passive_accept(struct selector_key * sk);
void pop3_write(struct selector_key * sk);
void pop3_read(struct selector_key * sk);
void pop3_close(struct selector_key * sk);
void pop3_block(struct selector_key * sk);
void pop3_done(struct selector_key * sk);

#endif
