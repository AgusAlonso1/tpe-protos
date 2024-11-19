#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "pop3.h"
#include "selector.h"
#include "pop3_parser.h"
#include "stm.h"
#include "buffer.h"

#define BUFFER_SIZE 2024
#define ERROR_CODE -1

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


/** definición de handlers para cada estado */
struct state_definition pop3_states_handler[] = {
        {
            .state            = WELCOME,
            .on_write_ready   = welcome_message,
        },
        {
            .state            = WAITING_USER,
            .on_arrival       = command_parser_clean,
            .on_read_ready    = waiting_user,             // TODO: espera a que se ingrese el usuario
            .on_write_ready   = waiting_user_response,    // TODO: imprime mensaje de bienvenida

        },
        {
            .state            = WAITING_PASS,
            .on_arrival       = command_parser_clean,
           //     .on_read_ready    = waiting_pass,             // TODO: espera a que se ingrese la contraseña
           //     .on_write_ready   = waiting_pass_response,    // TODO: imprime mensaje de bienvenida

        },
        {
            .state            = AUTH,
            .on_arrival       = command_parser_clean,
           //     .on_read_ready    = execute_command,          // TODO: analiza y ve el comando que deberia de ejecutar
           //     .on_write_ready   = execute_command,          // TODO: imprime mensaje de bienvenida

        },
        {
            .state = DONE,
        },
        {
            .state = ERROR,
        },
};

struct pop3_session_data {
    /** Información del cliente **/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    char * username;

    /** Maquina de estados **/
    struct state_machine stm;

    /** Buffers **/
    uint8_t buff_read[BUFFER_SIZE];
    uint8_t buff_write[BUFFER_SIZE];

    buffer buffer_read;
    buffer buffer_write;

    /** Parser **/
    struct pop3_command_parser parser;

};


/** ----------------------------- Definicion de funciones static ----------------------------- **/

static void pop3_write(struct selector_key * sk);
static void pop3_read(struct selector_key * sk);
static void pop3_done(struct selector_key * sk);
static void pop3_close(struct selector_key * sk);

static void command_parser_clean(struct selector_key * sk);
static unsigned read_command(struct selector_key * sk, pop3_session_data * session_data, unsigned current_state,unsigned next_state);
static bool process_command(pop3_session_data * session, unsigned current_state);
static unsigned welcome_message(struct selector_key * sk);
static unsigned waiting_user(struct selector_key * sk);
static unsigned waiting_user_response(struct selector_key * sk);


/** ----------------------------- Funciones del selector POP3 ----------------------------- **/

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

    struct pop3_session_data * pop3_session = malloc(sizeof (struct pop3_session_data));
    if(pop3_session == NULL) {
        goto fail;
    }

    /** Inicializamos con la información del cliente **/
    memset(pop3_session, 0, sizeof(*pop3_session));
    memcpy(&pop3_session->client_addr, &active_socket_addr, active_socket_addr_len);
    pop3_session->client_addr_len = active_socket_addr_len;

    /** Inicializamos los buffers **/
    buffer_init(&pop3_session->buffer_read, (size_t) BUFFER_SIZE, pop3_session->buff_read);
    buffer_init(&pop3_session->buffer_write,(size_t) BUFFER_SIZE, pop3_session->buff_write);

    /** Inicializamos la maquina de estados **/
    pop3_session->stm.initial = WELCOME;
    pop3_session->stm.max_state = ERROR;
    pop3_session->stm.states = pop3_states_handler;
    stm_init(&pop3_session->stm);

    /** Inicializamos el parser **/
    initialize_command_parser(&pop3_session->parser);

    const struct fd_handler pop3_handler = {
        .handle_read = pop3_read,
        .handle_write = pop3_write,
        .handle_close = pop3_close,
    };

    if (selector_register(sk->s, active_socket_fd, &pop3_handler, OP_WRITE, NULL) != SELECTOR_SUCCESS) {
        goto fail;
    }

    return;

    fail:
        if (active_socket_fd != ERROR_CODE) {
            close(active_socket_fd);
        }

        free(pop3_session);
}

void pop3_write(struct selector_key * sk) {
    struct state_machine * stm = &((struct pop3_session_data *)(sk)->data)->stm;
    const enum smtp_state st = stm_handler_write(stm, sk);

    if (ERROR == st || DONE == st) {
        pop3_done(sk);
    } else if (WAITING_USER == st || WAITING_PASS == st || AUTH == st) {
        buffer * rb = &((struct pop3_session_data *)(sk)->data)->buffer_read;
        if (buffer_can_read(rb)) {
            pop3_read(sk);
        }
    }
}

void pop3_read(struct selector_key * sk) {
    struct state_machine * stm = ((struct pop3_session_data *)(sk)->data)->stm;
    const enum pop3_state st = stm_handler_read(stm, sk);  // TODO: definir handler read del state machine

    if (ERROR == st || DONE == st) {
        pop3_done(sk);
    }
}

void pop3_done(struct selector_key * sk) {
    if (sk->fd != ERROR_CODE) {
        if (selector_unregister_fd(sk->s, sk->fd) != SELECTOR_SUCCESS) {
            abort();
        }
        close(sk->fd);
    }
}

void pop3_close(struct selector_key * sk) {
    struct pop3_session_data * pop3_session = ((struct pop3_session_data *)(sk)->data);
    free(pop3_session);
}



/** ----------------------------- Funciones de la maquina de estado POP3 ----------------------------- **/

/** Limpiamos el command parser para recibir y analizar un nuevo comando **/
static void command_parser_clean(struct selector_key * sk) {
    initialize_command_parser(&((struct pop3_session_data *) (sk->data))->parser);
}

/** Leemos el comando parseado **/
unsigned read_command(struct selector_key * sk, pop3_session_data * session, unsigned current_state,unsigned next_state) {
    bool error = false;
    unsigned command_state = consume_command(&session->buffer_read, &error);

    if(parsing_finished(command_state, &error)) {
        if(selector_set_interest_key(sk, OP_WRITE) != SELECTOR_SUCCESS) {
            return ERROR;
        } else {
            if(process_command(session, current_state)) {
                return next_state;
            } else {
                return DONE;
            }
        }
    }
    return current_state;
}

/** Imprimimos el mensaje de bienvenida **/
unsigned welcome_message(struct selector_key * sk) {
    unsigned current_state = ((struct pop3_session_data *) (sk)->data)->stm.current;

    size_t bytes_count;
    buffer * b_write = &((struct pop3_session_data *) (sk)->data)->buffer_write;

    /** Llenamos el buffer de escritura con el mensaje **/
    char *message = " 200 OK.\nWelcome to POPCORN <insert emoji> ";
    size_t message_len = strlen(message);
    memcpy(b_write->data, message, message_len);

    /** Avanzamos el puntero de escritura para indicar que el mensaje está en el buffer **/
    buffer_write_adv(b_write, message_len);

    /** Obtenemos el puntero de lectura y la cantidad de bytes disponibles para leer **/
    uint8_t *ptr = buffer_read_ptr(b_write, &bytes_count);

    /** Enviamos los datos a través del socket **/
    ssize_t bytes_sent = send(sk->fd, ptr, bytes_count, MSG_NOSIGNAL);

    if (bytes_sent >= 0) {
        /** Marcamos el mensaje como "leído" avanzando el puntero de lectura **/
        buffer_read_adv(b_write, bytes_sent);

        /** Si no hay más datos por leer, cambiamos el estado al siguiente **/
        if (!buffer_can_read(b_write)) {
            if (selector_set_interest_key(sk, OP_READ) != SELECTOR_SUCCESS) {
                return ERROR;
            } else {
                current_state = WAITING_USER;
            }
        } else {
            return ERROR;
        }
    }

    return current_state;
}

/** Esperamos que el cliente ingrese un usuario valido **/
unsigned waiting_user(struct selector_key * sk) {
    unsigned current_state = WAITING_USER;
    unsigned next_state = WAITING_PASS;
    pop3_session_data * session_data = (struct pop3_session_data *) sk->data;
    buffer * b_read = &session_data->buffer_read;

    if(buffer_can_read(b_read)) {
        return read_command(sk ,session_data, current_state ,next_state);
    } else {
        /** Si no hay suficientes datos en el buffer el código entonces intenta recibir más datos desde el socket con recv(). Esta función lee datos del socket y los coloca en el buffer. **/

        size_t count;
        uint8_t * ptr = buffer_write_ptr(&session_data->buffer_read, &count);

        /**
         *  @param fd -> identificador de un socket
         *  @param ptr -> puntero a un búfer en memoria donde se almacenarán los datos recibidos
         *  @param count -> define el tamaño del búfer
         *  @param MSG_DONTWAIT -> le indica que la llamada no debe bloquearse si no hay datos disponibles en el socket
        **/
        ssize_t bytes = recv(sk->fd, ptr, count, MSG_DONTWAIT);
        if(bytes > 0) {
            buffer_write_adv(&session_data->buffer_read, bytes);
            return read_command(sk ,session_data, current_state ,next_state);
        }
    }
    return current_state;
}

/** Imprimimos mensaje de ERROR o OK dependiendo del resultado de waiting_user **/
unsigned waiting_user_response(struct selector_key * sk) {

}

bool process_command(unsigned current_state, struct pop3_session_data * session) {
    switch (current_state) {
        case WAITING_USER:
            if(strcmp(session->parser.command->verb, "USER") == 0) {
                if(strlen(session->parser.command->arg1) == 0 || strspn(session->parser.command->arg, SPACE) == strlen(session->parser.command->arg) {
                    // TODO : agregar ERROR management
                }
                // TODO : validar el user
                // TODO : agregar SUCCESS management
            } else {
                /** Llenamos el buffer de escritura con el mensaje de ERROR **/
                buffer * b_write = session->buffer_write;
                char * message = " 500 ERROR. You should enter a valid user ";
                size_t message_len = strlen(message);
                memcpy(b_write->data, message, message_len);
            }
            break;
        case WAITING_PASS:
            break;
        case AUTH:
            break;
        case DONE:
            break;
        case ERROR:
            break;
        default:
            break;
    }

}






