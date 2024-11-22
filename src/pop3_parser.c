#include "pop3_parser.h"
#include "buffer.h"

/** -------------------------- Definición de funciones static  -------------------------- **/
static enum command_states get_token(uint8_t character, char *dest, size_t max_size, size_t *bytes_read, enum command_states next_state, enum command_states current_state);
static void finalize_token(char *dest, size_t *bytes_read);

/** Inicializamos el parser del comando **/
void initialize_command_parser(struct pop3_command_parser *parser) {
    if (parser->command == NULL) {
        parser->command = malloc(sizeof(*(parser->command)));
        if (parser->command == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for command\n");
            return;
        }
    }
    parser->state = verb;
    memset(parser->command, 0, sizeof(*(parser->command)));
    parser->bytes_read = 0;
}

/** Procesa un carácter según el estado actual del parser **/
enum command_states feed_character(uint8_t character, struct pop3_command_parser *parser) {
    switch (parser->state) {
        case verb:
            return get_token(character, parser->command->verb, sizeof(parser->command->verb), &parser->bytes_read, separator1, verb);
        case separator1:
            return (character == SPACE) ? arg1 : error;
        case arg1:
            return get_token(character, parser->command->arg1, sizeof(parser->command->arg1), &parser->bytes_read, separator2, arg1);
        case separator2:
            return (character == SPACE) ? arg2 : error;
        case arg2:
            return get_token(character, parser->command->arg2, sizeof(parser->command->arg2), &parser->bytes_read, eol, arg2);
        case eol:
            return (character == EOL) ? done : error;
        default:
            return error;
    }
}

/** Procesa un argumento (verbo o parámetro) **/
static enum command_states get_token(uint8_t character, char * dest, size_t max_size, size_t *bytes_read, enum command_states next_state, enum command_states current_state) {
    if (character == EOL) {
        finalize_token(dest, bytes_read);
        return eol;
    } else if (character == SPACE) {
        finalize_token(dest, bytes_read);
        return next_state;
    } else if (*bytes_read < max_size - 1) {
        dest[(*bytes_read)++] = character;
        return current_state;
    } else {
        return error;
    }
}

/** Finaliza el argumento escribiendo el terminador nulo y reinicia el contador **/
static void finalize_token(char *dest, size_t *bytes_read) {
    dest[*bytes_read] = '\0';
    *bytes_read = 0;
}

/** Se fija si se terminó de procesar el comando **/
bool parsing_finished(const enum command_states state, bool *errors) {
    if (state >= error && errors != NULL) {
        *errors = true;
    }
    return state >= done;
}

/** Procesa un comando POP3 desde un buffer **/
enum command_states consume_command(buffer *buffer, struct pop3_command_parser *parser, bool *errors) {
    while (buffer_can_read(buffer) && !parsing_finished(parser->state, errors)) {
        const uint8_t character = buffer_read(buffer);
        parser->state = feed_character(character, parser);

        if ((character == SPACE || character == EOL) && !parsing_finished(parser->state, errors)) {
            parser->state = feed_character(character, parser);
        }
    }
    return parser->state;
}
