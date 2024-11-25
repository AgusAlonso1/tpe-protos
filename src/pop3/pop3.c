#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pop3.h>
#include <stm.h>
#include <signal.h>
#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>

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
     *    - TRANSACTION cuando se completa el mensaje
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
    TRANSACTION,
    UPDATE,
    CLOSE_CONNECTION,
    ERROR
};

struct pop3_session_data {
    /** Información del cliente **/
    struct sockaddr_storage client_addr;
    socklen_t client_addr_len;
    char * username;

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

    /** Message manager **/
    struct mail_manager * m_manager;

};


static struct fd_handler pop3_handler = {
        .handle_read = pop3_read,
        .handle_write = pop3_write,
        .handle_close = pop3_close,
};

/** ----------------------------- Definición de funciones static ----------------------------- **/
static void command_parser_clean(unsigned state, struct selector_key * sk);
static unsigned read_transactional_command(struct selector_key *sk);
static unsigned write_transactional_command(struct selector_key *sk);
static unsigned read_command(struct selector_key * sk, struct pop3_session_data * session, unsigned current_state);
static bool process_command(struct selector_key * sk, unsigned current_state);
static unsigned handle_client_command(struct selector_key * sk);
unsigned handle_client_response(struct selector_key * sk);
static unsigned welcome_message(struct selector_key * sk);
static unsigned waiting_user(struct selector_key * sk);
static unsigned waiting_user_response(struct selector_key * sk);
static unsigned waiting_pass(struct selector_key * sk);
static unsigned waiting_pass_response(struct selector_key * sk);
static void process_quit(struct selector_key * sk);
static void process_dele(struct selector_key * sk, int number);
static void process_retr(struct selector_key *sk, int number);
static void process_list(struct selector_key *sk, int number);
static void process_stat(struct selector_key * sk);

static void checkout(unsigned state, struct selector_key * sk);
static unsigned goodbye_message(struct selector_key * sk);
static void close_connection(unsigned state, struct selector_key * sk);
static void destroy_session(struct selector_key * sk);

static bool write_file(buffer * b_write, FILE * message_file);
static void process_messages(unsigned state, struct selector_key * sk);
static void print_message(struct selector_key * sk, char * message);
static void write_message(struct selector_key * sk, char * message);






/** Definición de handlers para cada estado */
struct state_definition pop3_states_handler[] = {
        {
            .state            = WELCOME,
            .on_write_ready   = welcome_message,
        },
        {
            .state            = WAITING_USER,
         //   .on_arrival       = command_parser_clean,
            .on_read_ready    = waiting_user,
            .on_write_ready   = waiting_user_response,

        },
        {
            .state            = WAITING_PASS,
            .on_arrival       = command_parser_clean,
            .on_read_ready    = waiting_pass,
            .on_write_ready   = waiting_pass_response,
            .on_departure     = process_messages,

        },
        {
            .state            = TRANSACTION,
            .on_arrival       = command_parser_clean,
            .on_read_ready    = handle_client_command,          // TODO: analiza y ve el comando que debería de ejecutar
           .on_write_ready    = handle_client_response,          // TODO: imprime mensaje de bienvenida

        },
        {
            .state = UPDATE,
            .on_arrival = checkout,
            .on_write_ready = goodbye_message
        },
        {
                .state = CLOSE_CONNECTION,
        },
        {
            .state = ERROR,
        },
};


/** ----------------------------- Funciones del selector POP3 ----------------------------- **/

void pop3_passive_accept(struct selector_key * sk) {
    printf("Entramos a pop3_passive_accept\n");
    struct pop3_session_data * pop3_session = NULL;
    struct sockaddr_storage active_socket_addr;
    socklen_t active_socket_addr_len = sizeof(active_socket_addr);

    int active_socket_fd = accept(sk->fd, (struct sockaddr *) &active_socket_addr, &active_socket_addr_len);
    if (active_socket_fd == ERROR_CODE) {
        goto fail;
    }

    if (selector_fd_set_nio(active_socket_fd) == ERROR_CODE) {
        goto fail;
    }

    pop3_session = malloc(sizeof (struct pop3_session_data));
    if(pop3_session == NULL) {
        fprintf(stderr, "-Error: pop3_session is NULL\n");
        goto fail;
    }
    /** Inicializamos con la información del cliente **/
    memset(pop3_session, 0, sizeof(*pop3_session));
    memcpy(&pop3_session->client_addr, &active_socket_addr, active_socket_addr_len);
    pop3_session->client_addr_len = active_socket_addr_len;


    pop3_session->buff_write = malloc(BUFFER_SIZE);
    pop3_session->buff_read = malloc(BUFFER_SIZE);


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

    if (selector_register(sk->s, active_socket_fd, &pop3_handler, OP_WRITE, pop3_session) != SELECTOR_SUCCESS) {
        goto fail;
    }

    pop3_session->next_state = WELCOME;

    return;

    fail:
    if (active_socket_fd != ERROR_CODE) {
        close(active_socket_fd);
    }
    if(pop3_session != NULL) {
        free(pop3_session->parser.command);
        free(pop3_session->buff_read);
        free(pop3_session->buff_write);
        free(pop3_session);
    }
}

void pop3_write(struct selector_key * sk) {
    printf("Entramos a pop3_write\n");
    struct state_machine * stm = &((struct pop3_session_data *)(sk)->data)->stm;
    const enum command_states st = stm_handler_write(stm, sk);

    if (ERROR == st || CLOSE_CONNECTION == st) {
        pop3_done(sk);
    }
}

void pop3_read(struct selector_key * sk) {
    printf("Entramos a pop3_read\n");
    struct state_machine * stm = &((struct pop3_session_data *)(sk)->data)->stm;
    const enum pop3_state st = stm_handler_read(stm, sk);

    if (ERROR == st || CLOSE_CONNECTION == st) {
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
    struct pop3_session_data * session = ((struct pop3_session_data *)(sk)->data);
    stm_handler_close(&session->stm, sk);
    destroy_session(sk);
}

/** ----------------------------- Funciones de la maquina de estado POP3 ----------------------------- **/

/** Limpiamos el command parser para recibir y analizar un nuevo comando **/
static void command_parser_clean(unsigned state, struct selector_key * sk) {
    printf("Entramos a command_parser_clean\n");
    initialize_command_parser(&((struct pop3_session_data *)(sk->data))->parser);
}

/** Leemos el comando parseado **/
unsigned read_command(struct selector_key * sk, struct pop3_session_data * session, unsigned current_state) {
    printf("Entramos a read_command\n");
    bool error = false;
    unsigned command_state = consume_command(&session->buffer_read, &session->parser,&error);

    if(parsing_finished(command_state, &error)) {
        if(selector_set_interest_key(sk, OP_WRITE) != SELECTOR_SUCCESS) {
            return ERROR;
        }
        if(process_command(sk, current_state)) {
            return UPDATE;
        }
    }

    return current_state;
}

bool process_command(struct selector_key * sk, unsigned current_state) {    /** Si devuelve true quiere decir que hacemos QUIT **/
    printf("Entramos a process_command\n");
    struct pop3_session_data * session = (struct pop3_session_data *)(sk->data);
    char * message = NULL;

    switch (current_state) {
        case WAITING_USER:
            if(strcmp(session->parser.command->verb, USER) == 0) {
                if(strlen(session->parser.command->arg1) == 0) {
                    session->OK = false;
                    message = "-ERROR. Invalid USER \n";
                } else {

                    /** Llenamos el buffer de escritura con el mensaje de OK **/
                    session->OK = true;
                    session->next_state = WAITING_PASS;
                    session->username = strdup(session->parser.command->arg1);
                    message = "+OK. \n";
                    goto end;
                }
            } else if(strcmp(session->parser.command->verb, QUIT) == 0) {
                return true;
            } else {
                /** Llenamos el buffer de escritura con el mensaje de ERROR **/
                session->OK = false;
                message = "-ERROR. First enter USER \n";
            }
            break;
        case WAITING_PASS:
            if(strcmp(session->parser.command->verb, PASS) == 0) {
                if(strlen(session->parser.command->arg1) == 0){
                    session->OK = false;
                    message = "-ERROR. No password entered \n";
                } else {
                    // TODO : chequear que esta bien la contraseña

                    /** Llenamos el buffer de escritura con el mensaje de OK **/
                    session->OK = true;
                    session->next_state = TRANSACTION;
                    message = "+OK. \n";
                    goto end;
                }
            } else if(strcmp(session->parser.command->verb, USER) == 0) {
                if(strlen(session->parser.command->arg1) == 0) {
                    session->OK = false;
                    message = "-ERROR. Invalid USER \n";
                } else {
                    /** Llenamos el buffer de escritura con el mensaje de OK **/
                    session->OK = true;
                    session->next_state = WAITING_PASS;
                    session->username = strdup(session->parser.command->arg1);
                    message = "+OK. \n";
                    goto end;
                }
            } else if(strcmp(session->parser.command->verb, QUIT) == 0) {
                return true;
            } else {
                /** Llenamos el buffer de escritura con el mensaje de ERROR **/
                session->OK = false;
                message = "-ERROR. Invalid command \n";
            }
            break;
        case TRANSACTION:
            if (strcmp(session->parser.command->verb, STAT) == 0) {
                process_stat(session);
            } else if (strcmp(session->parser.command->verb, LIST) == 0) {
                int num = 0;
                if(strlen(session->parser.command->arg1) != 0){
                    num = atoi(session->parser.command->arg1);
                    if(num == 0) {
                        message = "-ERROR. Incorrect argument \n";
                    }
                }
                session->OK = false;
                process_list(sk, num);
            } else if (strcmp(session->parser.command->verb, RETR) == 0) {
                if(strlen(session->parser.command->arg1) == 0){
                        message = "-ERROR. Incorrect argument \n";
                } else {
                    int num = atoi(session->parser.command->arg1);
                    if(num == 0) {
                        message = "-ERROR. Incorrect argument \n";
                    }
                    session->OK = false;
                    process_retr(sk, num);
                }
            } else if (strcmp(session->parser.command->verb, DELE) == 0) {
                process_dele(sk, 0);
            } else if (strcmp(session->parser.command->verb, NOOP) == 0) {

            } else if (strcmp(session->parser.command->verb, RSET) == 0) {

            } else if (strcmp(session->parser.command->verb, QUIT) == 0) {
                return true;
            } else {
                session->OK = false;
                message = "-ERROR. Invalid command \n";
            }

            break;
        case UPDATE:
            break;
        case ERROR:
            break;
        default:
            break;
    }

    session->next_state = current_state;

    end:
    /** Llenamos el buffer de escritura con el mensaje de ERROR **/
    write_message(sk, message);
    return false;
}

unsigned handle_client_command(struct selector_key * sk) {
    printf("Entramos a handle_client_command\n");
    struct pop3_session_data * session = (struct pop3_session_data *) sk->data;
    unsigned current_state = session->stm.current->state;
    buffer * b_read = &session->buffer_read;

    if(buffer_can_read(b_read)) {
        return read_command(sk ,session, current_state);
    } else {

        /** Si no hay suficientes datos en el buffer el código entonces intenta recibir más datos desde el socket con recv(). Esta función lee datos del socket y los coloca en el buffer. **/
        size_t count;
        uint8_t * ptr = buffer_write_ptr(&session->buffer_read, &count);

        /**
         *  @param fd -> identificador de un socket
         *  @param ptr -> puntero a un búfer en memoria donde se almacenarán los datos recibidos
         *  @param count -> define el tamaño del búfer
         *  @param MSG_DONTWAIT -> le indica que la llamada no debe bloquearse si no hay datos disponibles en el socket
        **/
        ssize_t bytes = recv(sk->fd, ptr, count, MSG_DONTWAIT);
        if(bytes > 0) {
            buffer_write_adv(&session->buffer_read, bytes);
            return read_command(sk ,session, current_state);
        }
    }

    return current_state;
}

unsigned handle_client_response(struct selector_key * sk) {
    printf("Entramos a handle_client_response\n");
    struct pop3_session_data * session = ((struct pop3_session_data *) (sk)->data);

    unsigned current_state = session->stm.current->state;
    size_t bytes_count;
    buffer * b_write = &session->buffer_write;

    /** Obtenemos el puntero de lectura y la cantidad de bytes disponibles para leer **/
    uint8_t * ptr = buffer_read_ptr(b_write, &bytes_count);

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
                if(session->OK) {
                    current_state = session->next_state;
                } else {
                    command_parser_clean(session->stm.current->state, sk);
                }
            }
        } else {
            return ERROR;
        }
    }
    return current_state;
}

/** Imprimimos el mensaje de bienvenida **/
unsigned welcome_message(struct selector_key * sk) {
    printf("--> Entramos a welcome_message\n");
    struct pop3_session_data * session = ((struct pop3_session_data *) (sk)->data);
    unsigned current_state = session->stm.current->state;

    size_t bytes_count;
    buffer * b_write = &session->buffer_write;

    /** Llenamos el buffer de escritura con el mensaje **/
    char *message = "+OK.\nWelcome to POPCORN 🍿 \n";
    write_message(sk, message);

    /** Obtenemos el puntero de lectura y la cantidad de bytes disponibles para leer **/
    uint8_t * ptr = buffer_read_ptr(b_write, &bytes_count);

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
    printf("--> Entramos a waiting_user\n");
    return handle_client_command(sk);
}

/** Imprimimos mensaje de ERROR o OK dependiendo del resultado de waiting_user **/
unsigned waiting_user_response(struct selector_key * sk) {
    printf("--> Entramos a waiting_user_response\n");
    return handle_client_response(sk);
}

unsigned waiting_pass(struct selector_key * sk) {
    printf("--> Entramos a waiting_pass\n");
    return handle_client_command(sk);
}


/** Imprimimos mensaje de ERROR o OK dependiendo del resultado de waiting_pass **/
unsigned waiting_pass_response(struct selector_key * sk) {
    printf("--> Entramos a waiting_pass_response\n");
    return handle_client_response(sk);
}


// TODO : es exactamente igual a handle_client_command
unsigned read_transactional_command(struct selector_key *sk) {
    printf("Entramos a read_transactional_command\n");
    struct pop3_session_data *session = (struct pop3_session_data *) sk->data;
    buffer *b_read = &session->buffer_read;
    unsigned current_state = TRANSACTION;

    if(buffer_can_read(b_read)) {
        return read_command(sk ,session, current_state); //Saque next_state porque en el anterior tampoco se usa jeje
    } else {
        size_t count;
        uint8_t * ptr = buffer_write_ptr(&session->buffer_read, &count);
        ssize_t bytes = recv(sk->fd, ptr, count, MSG_DONTWAIT);
        if(bytes > 0) {
            buffer_write_adv(&session->buffer_read, bytes);
            return read_command(sk ,session, current_state);
        }
    }
    return current_state;
}

// TODO : es exactamente igual a handle_client_response
unsigned write_transactional_command(struct selector_key *sk) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    buffer *b_write = &session->buffer_write;

    size_t bytes_count;
    uint8_t *ptr = buffer_read_ptr(b_write, &bytes_count);
    ssize_t bytes_sent = send(sk->fd, ptr, bytes_count, MSG_NOSIGNAL);

    if (bytes_sent > 0) {
        buffer_read_adv(b_write, bytes_sent);
        if (!buffer_can_read(b_write)) {
            selector_set_interest(sk->s, sk->fd, OP_READ); //Chequear
            return TRANSACTION;
        }
    } else {
        return ERROR;
    }
    return TRANSACTION;
}

/** ----------------------------- Funciones de procesamiento de comandos POP3 ----------------------------- **/
void process_stat(struct selector_key * sk) {
    printf("Entramos a process_stat\n");

    // TODO : implementar
}

void process_list(struct selector_key * sk, int number) {
    printf("Entramos a process_list\n");

    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    char message[MESSAGE_SIZE];

    if(number == 0) {
        snprintf(message, sizeof(message), "+OK. %zu %zu \n", session->m_manager->messages_count, session->m_manager->messages_size);
        print_message(sk, message);

        for(int i = 0; i < session->m_manager->messages_count; i++) {
            snprintf(message, sizeof(message), " %d %zu \n", i + 1, session->m_manager->messages_array[i].size);
            print_message(sk, message);
        }
    } else {
        snprintf(message, sizeof(message), "+OK. %d %zu \n", number, session->m_manager->messages_array[number - 1].size);
        print_message(sk, message);
    }

}

void process_retr(struct selector_key *sk, int number) {
    printf("Entramos a process_retr\n");

    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    buffer *b_write = &session->buffer_write;
    char *error_message;
    size_t bytes_count;
    char message[MESSAGE_SIZE];
    int message_size;
    size_t octets;

    FILE *file = retrieve_message(session->m_manager, number, &message_size, &octets);

    if (file == NULL) {
        error_message = "+ERROR. Can not read file \n";
        write_message(sk, error_message);
        return;
    }

    snprintf(message, sizeof(message), "+OK. %zu octets\n", octets);
    print_message(sk, message);

    while (!feof(file)) {
        bool error = write_file(b_write, file);
        if (error) {
            fclose(file);
            break;
        }

        uint8_t *data_ptr = buffer_read_ptr(b_write, &bytes_count);
        ssize_t sent_bytes = send(sk->fd, data_ptr, bytes_count, MSG_NOSIGNAL | MSG_DONTWAIT);

        if (sent_bytes > 0) {
            buffer_read_adv(b_write, sent_bytes);
        }

        if (sent_bytes <= 0) {
            break;
        }
    }

    if (feof(file)) {
        fclose(file);
    }
}

void process_dele(struct selector_key * sk, int number) {
    printf("Entramos a process_dele\n");
    // TODO : implementar
}


static void checkout(unsigned state, struct selector_key * sk) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    session->OK = false;
    cleanup_deleted_messages(session->m_manager);

    char * message = "+OK. Logging out \n";
    write_message(sk, message);
}


static unsigned goodbye_message(struct selector_key * sk) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    session->OK = true;
    session->next_state = CLOSE_CONNECTION;

    return handle_client_response(sk);
}

static void destroy_session(struct selector_key * sk) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;

    /** Liberamos la Información del cliente **/
    free(session->username);

    /** Liberamos los Buffers **/
    free(session->buff_read);
    free(session->buff_write);

    /** Liberamos el mail manager **/
    free(session->m_manager);

    /** Liberamos el Parser **/
    free(session->parser.command);
    free(session);
}

void process_messages(unsigned state, struct selector_key * sk) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    session->m_manager = create_mail_manager("Maildir", session->username);
    if(session->m_manager == NULL) {
        session->next_state = ERROR;
    }
}

static void print_message(struct selector_key * sk, char * message) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    size_t bytes_count;
    size_t message_len;
    buffer * b_write = &session->buffer_write;

    message_len = strlen(message);
    memcpy(b_write->data, message, message_len);
    buffer_write_adv(b_write, message_len);
    uint8_t *ptr = buffer_read_ptr(b_write, &bytes_count);
    ssize_t bytes_sent = send(sk->fd, ptr, bytes_count, MSG_NOSIGNAL);

    if (bytes_sent > 0) {
        buffer_read_adv(b_write, bytes_sent);
    }
}

static void write_message(struct selector_key * sk, char * message) {
    struct pop3_session_data *session = (struct pop3_session_data *)sk->data;
    size_t message_len;
    buffer * b_write = &session->buffer_write;

    if(message != NULL) {
        message_len = strlen(message);
        memcpy(b_write->data, message, message_len);
        buffer_write_adv(b_write, message_len);
    }
}

bool write_file(buffer * b_write, FILE * message_file) {
    if (b_write == NULL || message_file == NULL) {
        return true;
    }

    char chunk[1024];
    size_t bytes_read = fread(chunk, 1, sizeof(chunk), message_file);

    if (bytes_read == 0) {
        return true;
    }

    size_t bytes_available;
    uint8_t *ptr = buffer_read_ptr(b_write, &bytes_available);

    if (ptr != NULL) {
        size_t copy_size = (bytes_read < bytes_available) ? bytes_read : bytes_available;
        memcpy(ptr, chunk, copy_size);
        buffer_read_adv(b_write, copy_size);
    }

    return false;
}











